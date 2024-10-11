#include "tablemodel.h"

#include <QSet>

#include "component/constvalue.h"
#include "global/resourcepool.h"

TableModel::TableModel(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , rule_ { rule }
    , info_ { info }
    , node_id_ { node_id }
{
    if (node_id >= 1)
        sql_->ReadTrans(trans_shadow_list_, node_id);
}

TableModel::~TableModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void TableModel::RRemoveMultiTrans(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    auto trans_id_list { node_trans.values(node_id_) };
    RemoveMultiTrans(QSet(trans_id_list.cbegin(), trans_id_list.cend()));
}

void TableModel::RMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMultiTrans(QSet(trans_id_list.cbegin(), trans_id_list.cend()));

    if (node_id_ == new_node_id)
        AppendMultiTrans(node_id_, trans_id_list);
}

void TableModel::RRule(int node_id, bool rule)
{
    if (node_id_ != node_id || rule_ == rule)
        return;

    for (auto* trans_shadow : trans_shadow_list_)
        trans_shadow->subtotal = -trans_shadow->subtotal;

    rule_ = rule;
}

void TableModel::RAppendOneTrans(const TransShadow* trans_shadow)
{
    if (node_id_ != *trans_shadow->rhs_node)
        return;

    auto new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;
    new_trans_shadow->unit_price = trans_shadow->unit_price;
    new_trans_shadow->discount_price = trans_shadow->discount_price;
    new_trans_shadow->settled = trans_shadow->settled;
    new_trans_shadow->node_id = trans_shadow->node_id;

    new_trans_shadow->rhs_ratio = trans_shadow->lhs_ratio;
    new_trans_shadow->rhs_debit = trans_shadow->lhs_debit;
    new_trans_shadow->rhs_credit = trans_shadow->lhs_credit;
    new_trans_shadow->rhs_node = trans_shadow->lhs_node;

    new_trans_shadow->lhs_node = trans_shadow->rhs_node;
    new_trans_shadow->lhs_ratio = trans_shadow->rhs_ratio;
    new_trans_shadow->lhs_debit = trans_shadow->rhs_debit;
    new_trans_shadow->lhs_credit = trans_shadow->rhs_credit;

    auto row { trans_shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.emplaceBack(new_trans_shadow);
    endInsertRows();

    double previous_balance { row >= 1 ? trans_shadow_list_.at(row - 1)->subtotal : 0.0 };
    new_trans_shadow->subtotal = Balance(rule_, *new_trans_shadow->lhs_debit, *new_trans_shadow->lhs_credit) + previous_balance;
}

void TableModel::RRemoveOneTrans(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(row));
    endRemoveRows();

    RunAccumulateSubtotal(row, rule_);
}

void TableModel::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        RunAccumulateSubtotal(index.row(), rule_);
}

bool TableModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int rhs_node_id { *trans_shadow->rhs_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (rhs_node_id != 0) {
        auto ratio { *trans_shadow->lhs_ratio };
        auto debit { *trans_shadow->lhs_debit };
        auto credit { *trans_shadow->lhs_credit };
        emit SUpdateLeafValue(node_id_, -debit, -credit, -ratio * debit, -ratio * credit);

        ratio = *trans_shadow->rhs_ratio;
        debit = *trans_shadow->rhs_debit;
        credit = *trans_shadow->rhs_credit;
        emit SUpdateLeafValue(*trans_shadow->rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, rhs_node_id, trans_id);
        RunAccumulateSubtotal(row, rule_);

        sql_->RemoveTrans(trans_id);
    }

    ResourcePool<TransShadow>::Instance().Recycle(trans_shadow);
    return true;
}

void TableModel::UpdateAllState(Check state)
{
    for (auto* trans_shadow : trans_shadow_list_) {
        switch (state) {
        case Check::kAll:
            *trans_shadow->state = true;
            break;
        case Check::kNone:
            *trans_shadow->state = false;
            break;
        case Check::kReverse:
            *trans_shadow->state = !*trans_shadow->state;
            break;
        default:
            break;
        }
    }

    switch (state) {
    case Check::kAll:
        sql_->UpdateCheckState("state", true, state);
        break;
    case Check::kNone:
        sql_->UpdateCheckState("state", false, state);
        break;
    case Check::kReverse:
        sql_->UpdateCheckState("state", true, state);
        break;
    default:
        break;
    }

    int column { std::to_underlying(TableEnum::kState) };
    emit dataChanged(index(0, column), index(rowCount() - 1, column));
}

bool TableModel::UpdateDebit(TransShadow* trans_shadow, double value)
{
    double lhs_debit { *trans_shadow->lhs_debit };
    if (std::abs(lhs_debit - value) < TOLERANCE)
        return false;

    double lhs_credit { *trans_shadow->lhs_credit };
    double lhs_ratio { *trans_shadow->lhs_ratio };

    double abs { qAbs(value - lhs_credit) };
    *trans_shadow->lhs_debit = (value > lhs_credit) ? abs : 0;
    *trans_shadow->lhs_credit = (value <= lhs_credit) ? abs : 0;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(*trans_shadow->lhs_node, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff * lhs_ratio, lhs_credit_diff * lhs_ratio);

    auto rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    auto rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

bool TableModel::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double lhs_credit { *trans_shadow->lhs_credit };
    if (std::abs(lhs_credit - value) < TOLERANCE)
        return false;

    double lhs_debit { *trans_shadow->lhs_debit };
    double lhs_ratio { *trans_shadow->lhs_ratio };

    double abs { qAbs(value - lhs_debit) };
    *trans_shadow->lhs_debit = (value > lhs_debit) ? 0 : abs;
    *trans_shadow->lhs_credit = (value <= lhs_debit) ? 0 : abs;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit = (*trans_shadow->lhs_credit) * lhs_ratio / rhs_ratio;
    *trans_shadow->rhs_credit = (*trans_shadow->lhs_debit) * lhs_ratio / rhs_ratio;

    if (*trans_shadow->rhs_node == 0)
        return false;

    auto lhs_debit_diff { *trans_shadow->lhs_debit - lhs_debit };
    auto lhs_credit_diff { *trans_shadow->lhs_credit - lhs_credit };
    emit SUpdateLeafValue(*trans_shadow->lhs_node, lhs_debit_diff, lhs_credit_diff, lhs_debit_diff * lhs_ratio, lhs_credit_diff * lhs_ratio);

    auto rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    auto rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

bool TableModel::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double lhs_ratio { *trans_shadow->lhs_ratio };

    if (std::abs(lhs_ratio - value) < TOLERANCE || value <= 0)
        return false;

    auto diff { value - lhs_ratio };
    auto proportion { value / *trans_shadow->lhs_ratio };

    *trans_shadow->lhs_ratio = value;

    double rhs_debit { *trans_shadow->rhs_debit };
    double rhs_credit { *trans_shadow->rhs_credit };
    double rhs_ratio { *trans_shadow->rhs_ratio };

    *trans_shadow->rhs_debit *= proportion;
    *trans_shadow->rhs_credit *= proportion;

    if (*trans_shadow->rhs_node == 0)
        return false;

    emit SUpdateLeafValue(*trans_shadow->lhs_node, 0, 0, *trans_shadow->lhs_debit * diff, *trans_shadow->lhs_credit * diff);

    auto rhs_debit_diff { *trans_shadow->rhs_debit - rhs_debit };
    auto rhs_credit_diff { *trans_shadow->rhs_credit - rhs_credit };
    emit SUpdateLeafValue(*trans_shadow->rhs_node, rhs_debit_diff, rhs_credit_diff, rhs_debit_diff * rhs_ratio, rhs_credit_diff * rhs_ratio);

    return true;
}

QModelIndex TableModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TableModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int TableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_shadow_list_.size();
}

int TableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_.table_header.size();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.table_header.at(section);

    return QVariant();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans_shadow { trans_shadow_list_.at(index.row()) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
        return *trans_shadow->id;
    case TableEnum::kDateTime:
        return *trans_shadow->date_time;
    case TableEnum::kCode:
        return *trans_shadow->code;
    case TableEnum::kLhsRatio:
        return *trans_shadow->lhs_ratio;
    case TableEnum::kDescription:
        return *trans_shadow->description;
    case TableEnum::kRhsNode:
        return *trans_shadow->rhs_node == 0 ? QVariant() : *trans_shadow->rhs_node;
    case TableEnum::kState:
        return *trans_shadow->state;
    case TableEnum::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    case TableEnum::kDebit:
        return *trans_shadow->lhs_debit == 0 ? QVariant() : *trans_shadow->lhs_debit;
    case TableEnum::kCredit:
        return *trans_shadow->lhs_credit == 0 ? QVariant() : *trans_shadow->lhs_credit;
    case TableEnum::kSubtotal:
        return trans_shadow->subtotal;
    default:
        return QVariant();
    }
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto trans_shadow { trans_shadow_list_.at(kRow) };
    int old_rhs_node { *trans_shadow->rhs_node };

    bool rhs_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool rat_changed { false };

    switch (kColumn) {
    case TableEnum::kDateTime:
        UpdateField(trans_shadow, value.toString(), DATE_TIME, &TransShadow::date_time);
        break;
    case TableEnum::kCode:
        UpdateField(trans_shadow, value.toString(), CODE, &TransShadow::code);
        break;
    case TableEnum::kState:
        UpdateField(trans_shadow, value.toBool(), STATE, &TransShadow::state);
        break;
    case TableEnum::kDescription:
        UpdateField(trans_shadow, value.toString(), DESCRIPTION, &TransShadow::description, [this]() { emit SSearch(); });
        break;
    case TableEnum::kLhsRatio:
        rat_changed = UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnum::kRhsNode:
        rhs_changed = UpdateRhsNode(trans_shadow, value.toInt());
        break;
    case TableEnum::kDebit:
        deb_changed = UpdateDebit(trans_shadow, value.toDouble());
        break;
    case TableEnum::kCredit:
        cre_changed = UpdateCredit(trans_shadow, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_rhs_node == 0) {
        if (rhs_changed) {
            sql_->WriteTrans(trans_shadow);
            RunAccumulateSubtotal(kRow, rule_);

            emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
            emit SAppendOneTrans(info_.section, trans_shadow);

            auto ratio { *trans_shadow->lhs_ratio };
            auto debit { *trans_shadow->lhs_debit };
            auto credit { *trans_shadow->lhs_credit };
            emit SUpdateLeafValue(node_id_, debit, credit, ratio * debit, ratio * credit);

            ratio = *trans_shadow->rhs_ratio;
            debit = *trans_shadow->rhs_debit;
            credit = *trans_shadow->rhs_credit;
            emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SSearch();
        emit SUpdateBalance(info_.section, old_rhs_node, *trans_shadow->id);
    }

    if (deb_changed || cre_changed) {
        RunAccumulateSubtotal(kRow, rule_);
        emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
    }

    if (rhs_changed) {
        sql_->UpdateTransValue(trans_shadow);
        emit SRemoveOneTrans(info_.section, old_rhs_node, *trans_shadow->id);
        emit SAppendOneTrans(info_.section, trans_shadow);

        auto ratio { *trans_shadow->rhs_ratio };
        auto debit { *trans_shadow->rhs_debit };
        auto credit { *trans_shadow->rhs_credit };
        emit SUpdateLeafValue(*trans_shadow->rhs_node, debit, credit, ratio * debit, ratio * credit);
        emit SUpdateLeafValue(old_rhs_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModel::sort(int column, Qt::SortOrder order)
{ // ignore subtotal column
    if (column <= -1 || column >= info_.table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnum kColumn { column };

        switch (kColumn) {
        case TableEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnum::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnum::kLhsRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_ratio < *rhs->lhs_ratio) : (*lhs->lhs_ratio > *rhs->lhs_ratio);
        case TableEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnum::kRhsNode:
            return (order == Qt::AscendingOrder) ? (*lhs->rhs_node < *rhs->rhs_node) : (*lhs->rhs_node > *rhs->rhs_node);
        case TableEnum::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnum::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_debit < *rhs->lhs_debit) : (*lhs->lhs_debit > *rhs->lhs_debit);
        case TableEnum::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->lhs_credit < *rhs->lhs_credit) : (*lhs->lhs_credit > *rhs->lhs_credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();

    RunAccumulateSubtotal(0, rule_);
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
    case TableEnum::kSubtotal:
    case TableEnum::kDocument:
    case TableEnum::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}

int TableModel::GetRhsNodeRow(int rhs_node_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->rhs_node == rhs_node_id) {
            return row;
        }
        ++row;
    }
    return -1;
}

int TableModel::GetLhsNodeRow(int lhs_node_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->lhs_node == lhs_node_id) {
            return row;
        }
        ++row;
    }
    return -1;
}

QModelIndex TableModel::GetIndex(int trans_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->id == trans_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}

QStringList* TableModel::GetDocumentPointer(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= trans_shadow_list_.size()) {
        qWarning() << "Invalid QModelIndex provided.";
        return nullptr;
    }

    auto trans_shadow { trans_shadow_list_[index.row()] };

    if (!trans_shadow || !trans_shadow->document) {
        qWarning() << "Null pointer encountered in trans_list_ or document.";
        return nullptr;
    }

    return trans_shadow->document;
}

bool TableModel::insertRows(int row, int /*count*/, const QModelIndex& parent)
{
    // just register trans_shadow in this function
    // while set rhs node in setData function, register trans to sql_'s trans_hash_
    auto trans_shadow { sql_->AllocateTransShadow() };

    *trans_shadow->lhs_node = node_id_;

    beginInsertRows(parent, row, row);
    trans_shadow_list_.emplaceBack(trans_shadow);
    endInsertRows();

    return true;
}

void TableModel::AccumulateSubtotal(int start, bool rule)
{
    if (start <= -1 || start >= trans_shadow_list_.size() || trans_shadow_list_.isEmpty())
        return;

    double previous_subtotal { start >= 1 ? trans_shadow_list_.at(start - 1)->subtotal : 0.0 };

    std::accumulate(trans_shadow_list_.begin() + start, trans_shadow_list_.end(), previous_subtotal, [&](double current_subtotal, TransShadow* trans_shadow) {
        trans_shadow->subtotal = Balance(rule, *trans_shadow->lhs_debit, *trans_shadow->lhs_credit) + current_subtotal;
        return trans_shadow->subtotal;
    });
}

bool TableModel::UpdateRhsNode(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->rhs_node == value)
        return false;

    *trans_shadow->rhs_node = value;
    return true;
}

bool TableModel::RemoveMultiTrans(const QSet<int>& trans_id_set)
{
    if (trans_id_set.isEmpty())
        return false;

    int min_row { -1 };
    int trans_id {};

    for (int i = trans_shadow_list_.size() - 1; i >= 0; --i) {
        trans_id = *trans_shadow_list_.at(i)->id;

        if (trans_id_set.contains(trans_id)) {
            if (min_row == -1 || i < min_row)
                min_row = i;

            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_.takeAt(i));
            endRemoveRows();
        }
    }

    if (min_row != -1)
        RunAccumulateSubtotal(min_row, rule_);

    return true;
}

bool TableModel::AppendMultiTrans(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->ReadTransRange(trans_shadow_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    RunAccumulateSubtotal(row, rule_);

    return true;
}
