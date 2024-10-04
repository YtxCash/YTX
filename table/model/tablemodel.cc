#include "tablemodel.h"

#include <QSet>
#include <QtConcurrent>

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
        sql_->BuildTransShadowList(trans_shadow_list_, node_id);
}

TableModel::~TableModel() { ResourcePool<TransShadow>::Instance().Recycle(trans_shadow_list_); }

void TableModel::RRemoveMultiTrans(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    auto trans_id_list { node_trans.values(node_id_) };
    RemoveMulti(QSet(trans_id_list.cbegin(), trans_id_list.cend()));
}

void TableModel::RMoveMultiTrans(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMulti(QSet(trans_id_list.cbegin(), trans_id_list.cend()));

    if (node_id_ == new_node_id)
        AppendMulti(node_id_, trans_id_list);
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
    if (node_id_ != *trans_shadow->related_node)
        return;

    auto new_trans_shadow { ResourcePool<TransShadow>::Instance().Allocate() };
    new_trans_shadow->date_time = trans_shadow->date_time;
    new_trans_shadow->id = trans_shadow->id;
    new_trans_shadow->description = trans_shadow->description;
    new_trans_shadow->code = trans_shadow->code;
    new_trans_shadow->document = trans_shadow->document;
    new_trans_shadow->state = trans_shadow->state;

    new_trans_shadow->related_ratio = trans_shadow->ratio;
    new_trans_shadow->related_debit = trans_shadow->debit;
    new_trans_shadow->related_credit = trans_shadow->credit;
    new_trans_shadow->related_node = trans_shadow->node;

    new_trans_shadow->node = trans_shadow->related_node;
    new_trans_shadow->ratio = trans_shadow->related_ratio;
    new_trans_shadow->debit = trans_shadow->related_debit;
    new_trans_shadow->credit = trans_shadow->related_credit;

    auto row { trans_shadow_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_shadow_list_.emplaceBack(new_trans_shadow);
    endInsertRows();

    double previous_balance { row >= 1 ? trans_shadow_list_.at(row - 1)->subtotal : 0.0 };
    new_trans_shadow->subtotal = Balance(rule_, *new_trans_shadow->debit, *new_trans_shadow->credit) + previous_balance;
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

    QtConcurrent::run(&TableModel::AccumulateSubtotal, this, row, rule_);
}

void TableModel::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        QtConcurrent::run(&TableModel::AccumulateSubtotal, this, index.row(), rule_);
}

bool TableModel::removeRows(int row, int /*count*/, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans_shadow { trans_shadow_list_.at(row) };
    int related_node_id { *trans_shadow->related_node };

    beginRemoveRows(parent, row, row);
    trans_shadow_list_.removeAt(row);
    endRemoveRows();

    if (related_node_id != 0) {
        auto ratio { *trans_shadow->ratio };
        auto debit { *trans_shadow->debit };
        auto credit { *trans_shadow->credit };
        emit SUpdateLeafTotal(node_id_, -debit, -credit, -ratio * debit, -ratio * credit);

        ratio = *trans_shadow->related_ratio;
        debit = *trans_shadow->related_debit;
        credit = *trans_shadow->related_credit;
        emit SUpdateLeafTotal(*trans_shadow->related_node, -debit, -credit, -ratio * debit, -ratio * credit);

        int trans_id { *trans_shadow->id };
        emit SRemoveOneTrans(info_.section, related_node_id, trans_id);
        QtConcurrent::run(&TableModel::AccumulateSubtotal, this, row, rule_);

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
    double debit { *trans_shadow->debit };
    if (std::abs(debit - value) < TOLERANCE)
        return false;

    double credit { *trans_shadow->credit };
    double ratio { *trans_shadow->ratio };

    double abs { qAbs(value - credit) };
    *trans_shadow->debit = (value > credit) ? abs : 0;
    *trans_shadow->credit = (value <= credit) ? abs : 0;

    double related_debit { *trans_shadow->related_debit };
    double related_credit { *trans_shadow->related_credit };
    double related_ratio { *trans_shadow->related_ratio };

    *trans_shadow->related_debit = (*trans_shadow->credit) * ratio / related_ratio;
    *trans_shadow->related_credit = (*trans_shadow->debit) * ratio / related_ratio;

    if (*trans_shadow->related_node == 0)
        return false;

    auto initial_debit_diff { *trans_shadow->debit - debit };
    auto initial_credit_diff { *trans_shadow->credit - credit };
    emit SUpdateLeafTotal(*trans_shadow->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto related_initial_debit_diff { *trans_shadow->related_debit - related_debit };
    auto related_initial_credit_diff { *trans_shadow->related_credit - related_credit };
    emit SUpdateLeafTotal(*trans_shadow->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
        related_initial_credit_diff * related_ratio);

    return true;
}

bool TableModel::UpdateCredit(TransShadow* trans_shadow, double value)
{
    double credit { *trans_shadow->credit };
    if (std::abs(credit - value) < TOLERANCE)
        return false;

    double debit { *trans_shadow->debit };
    double ratio { *trans_shadow->ratio };

    double abs { qAbs(value - debit) };
    *trans_shadow->debit = (value > debit) ? 0 : abs;
    *trans_shadow->credit = (value <= debit) ? 0 : abs;

    double related_debit { *trans_shadow->related_debit };
    double related_credit { *trans_shadow->related_credit };
    double related_ratio { *trans_shadow->related_ratio };

    *trans_shadow->related_debit = (*trans_shadow->credit) * ratio / related_ratio;
    *trans_shadow->related_credit = (*trans_shadow->debit) * ratio / related_ratio;

    if (*trans_shadow->related_node == 0)
        return false;

    auto initial_debit_diff { *trans_shadow->debit - debit };
    auto initial_credit_diff { *trans_shadow->credit - credit };
    emit SUpdateLeafTotal(*trans_shadow->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto related_initial_debit_diff { *trans_shadow->related_debit - related_debit };
    auto related_initial_credit_diff { *trans_shadow->related_credit - related_credit };
    emit SUpdateLeafTotal(*trans_shadow->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
        related_initial_credit_diff * related_ratio);

    return true;
}

bool TableModel::UpdateRatio(TransShadow* trans_shadow, double value)
{
    double ratio { *trans_shadow->ratio };

    if (std::abs(ratio - value) < TOLERANCE || value <= 0)
        return false;

    auto result { value - ratio };
    auto proportion { value / *trans_shadow->ratio };

    *trans_shadow->ratio = value;

    double related_debit { *trans_shadow->related_debit };
    double related_credit { *trans_shadow->related_credit };
    double related_ratio { *trans_shadow->related_ratio };

    *trans_shadow->related_debit *= proportion;
    *trans_shadow->related_credit *= proportion;

    if (*trans_shadow->related_node == 0)
        return false;

    emit SUpdateLeafTotal(*trans_shadow->node, 0, 0, *trans_shadow->debit * result, *trans_shadow->credit * result);

    auto related_initial_debit_diff { *trans_shadow->related_debit - related_debit };
    auto related_initial_credit_diff { *trans_shadow->related_credit - related_credit };
    emit SUpdateLeafTotal(*trans_shadow->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
        related_initial_credit_diff * related_ratio);

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
    return info_.part_table_header.size();
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_.part_table_header.at(section);

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
    case TableEnum::kRatio:
        return *trans_shadow->ratio;
    case TableEnum::kDescription:
        return *trans_shadow->description;
    case TableEnum::kRelatedNode:
        return *trans_shadow->related_node == 0 ? QVariant() : *trans_shadow->related_node;
    case TableEnum::kState:
        return *trans_shadow->state;
    case TableEnum::kDocument:
        return trans_shadow->document->isEmpty() ? QVariant() : QString::number(trans_shadow->document->size());
    case TableEnum::kDebit:
        return *trans_shadow->debit == 0 ? QVariant() : *trans_shadow->debit;
    case TableEnum::kCredit:
        return *trans_shadow->credit == 0 ? QVariant() : *trans_shadow->credit;
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
    int old_related_node { *trans_shadow->related_node };

    bool rel_changed { false };
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
    case TableEnum::kRatio:
        rat_changed = UpdateRatio(trans_shadow, value.toDouble());
        break;
    case TableEnum::kRelatedNode:
        rel_changed = UpdateRelatedNode(trans_shadow, value.toInt());
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

    if (old_related_node == 0) {
        if (rel_changed) {
            sql_->InsertTransShadow(trans_shadow);
            QtConcurrent::run(&TableModel::AccumulateSubtotal, this, kRow, rule_);

            emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
            emit SAppendOneTrans(info_.section, trans_shadow);

            auto ratio { *trans_shadow->ratio };
            auto debit { *trans_shadow->debit };
            auto credit { *trans_shadow->credit };
            emit SUpdateLeafTotal(node_id_, debit, credit, ratio * debit, ratio * credit);

            ratio = *trans_shadow->related_ratio;
            debit = *trans_shadow->related_debit;
            credit = *trans_shadow->related_credit;
            emit SUpdateLeafTotal(*trans_shadow->related_node, debit, credit, ratio * debit, ratio * credit);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->UpdateTrans(*trans_shadow->id);
        emit SSearch();
        emit SUpdateBalance(info_.section, old_related_node, *trans_shadow->id);
    }

    if (deb_changed || cre_changed) {
        QtConcurrent::run(&TableModel::AccumulateSubtotal, this, kRow, rule_);
        emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
    }

    if (rel_changed) {
        sql_->UpdateTrans(*trans_shadow->id);
        emit SRemoveOneTrans(info_.section, old_related_node, *trans_shadow->id);
        emit SAppendOneTrans(info_.section, trans_shadow);

        auto ratio { *trans_shadow->related_ratio };
        auto debit { *trans_shadow->related_debit };
        auto credit { *trans_shadow->related_credit };
        emit SUpdateLeafTotal(*trans_shadow->related_node, debit, credit, ratio * debit, ratio * credit);
        emit SUpdateLeafTotal(old_related_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModel::sort(int column, Qt::SortOrder order)
{ // ignore subtotal column
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](TransShadow* lhs, TransShadow* rhs) -> bool {
        const TableEnum kColumn { column };

        switch (kColumn) {
        case TableEnum::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case TableEnum::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case TableEnum::kRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->ratio < *rhs->ratio) : (*lhs->ratio > *rhs->ratio);
        case TableEnum::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case TableEnum::kRelatedNode:
            return (order == Qt::AscendingOrder) ? (*lhs->related_node < *rhs->related_node) : (*lhs->related_node > *rhs->related_node);
        case TableEnum::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case TableEnum::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case TableEnum::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->debit < *rhs->debit) : (*lhs->debit > *rhs->debit);
        case TableEnum::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->credit < *rhs->credit) : (*lhs->credit > *rhs->credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_shadow_list_.begin(), trans_shadow_list_.end(), Compare);
    emit layoutChanged();

    QtConcurrent::run(&TableModel::AccumulateSubtotal, this, 0, rule_);
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

int TableModel::GetRow(int node_id) const
{
    int row { 0 };

    for (const auto* trans_shadow : trans_shadow_list_) {
        if (*trans_shadow->related_node == node_id) {
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
    // while set related node in setData function, register trans to sql_'s trans_hash_
    auto trans_shadow { sql_->AllocateTransShadow() };

    *trans_shadow->node = node_id_;

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
        trans_shadow->subtotal = Balance(rule, *trans_shadow->debit, *trans_shadow->credit) + current_subtotal;
        return trans_shadow->subtotal;
    });
}

bool TableModel::UpdateRelatedNode(TransShadow* trans_shadow, int value)
{
    if (*trans_shadow->related_node == value)
        return false;

    *trans_shadow->related_node = value;
    return true;
}

bool TableModel::RemoveMulti(const QSet<int>& trans_id_set)
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
        QtConcurrent::run(&TableModel::AccumulateSubtotal, this, min_row, rule_);

    return true;
}

bool TableModel::AppendMulti(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_shadow_list_.size() };
    TransShadowList trans_shadow_list {};

    sql_->BuildTransShadowList(trans_shadow_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_shadow_list.size() - 1);
    trans_shadow_list_.append(trans_shadow_list);
    endInsertRows();

    QtConcurrent::run(&TableModel::AccumulateSubtotal, this, row, rule_);

    return true;
}
