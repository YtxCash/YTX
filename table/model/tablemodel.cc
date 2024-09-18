#include "tablemodel.h"

#include "global/resourcepool.h"

TableModel::TableModel(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : QAbstractItemModel(parent)
    , sql_ { sql }
    , node_rule_ { node_rule }
    , info_ { info }
    , section_rule_ { section_rule }
    , node_id_ { node_id }
{
    sql_->BuildTransList(trans_list_, node_id);
}

TableModel::~TableModel() { ResourcePool<Trans>::Instance().Recycle(trans_list_); }

void TableModel::RRemoveMulti(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    auto trans_list { node_trans.values(node_id_) };
    RemoveMulti(trans_list);
}

void TableModel::RNodeRule(int node_id, bool node_rule)
{
    if (node_id_ != node_id || node_rule_ == node_rule)
        return;

    for (auto& trans : trans_list_)
        trans->subtotal = -trans->subtotal;

    node_rule_ = node_rule;
}

void TableModel::RAppendOne(const Trans* trans)
{
    if (node_id_ != *trans->related_node)
        return;

    auto new_trans { ResourcePool<Trans>::Instance().Allocate() };
    new_trans->date_time = trans->date_time;
    new_trans->id = trans->id;
    new_trans->description = trans->description;
    new_trans->code = trans->code;
    new_trans->document = trans->document;
    new_trans->state = trans->state;

    new_trans->related_ratio = trans->ratio;
    new_trans->related_debit = trans->debit;
    new_trans->related_credit = trans->credit;
    new_trans->related_node = trans->node;

    new_trans->node = trans->related_node;
    new_trans->ratio = trans->related_ratio;
    new_trans->debit = trans->related_debit;
    new_trans->credit = trans->related_credit;

    auto row { trans_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_list_.emplaceBack(new_trans);
    endInsertRows();

    double previous_balance { row >= 1 ? trans_list_.at(row - 1)->subtotal : 0.0 };
    new_trans->subtotal = Balance(node_rule_, *new_trans->debit, *new_trans->credit) + previous_balance;
}

void TableModel::RRetrieveOne(Trans* trans)
{
    auto row { trans_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_list_.emplaceBack(trans);
    endInsertRows();

    double previous_balance { row >= 1 ? trans_list_.at(row - 1)->subtotal : 0.0 };
    trans->subtotal = Balance(node_rule_, *trans->debit, *trans->credit) + previous_balance;
}

void TableModel::RRemoveOne(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto idx { GetIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    ResourcePool<Trans>::Instance().Recycle(trans_list_.takeAt(row));
    endRemoveRows();

    AccumulateSubtotal(row, node_rule_);
}

void TableModel::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { GetIndex(trans_id) };
    if (index.isValid())
        AccumulateSubtotal(index.row(), node_rule_);
}

void TableModel::RMoveMulti(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMulti(trans_id_list);

    if (node_id_ == new_node_id)
        InsertMulti(new_node_id, trans_id_list);
}

bool TableModel::RemoveTrans(int row, const QModelIndex& parent)
{
    if (row <= -1)
        return false;

    auto trans { trans_list_.at(row) };
    int related_node_id { *trans->related_node };

    beginRemoveRows(parent, row, row);
    trans_list_.removeAt(row);
    endRemoveRows();

    if (related_node_id != 0) {
        auto ratio { *trans->ratio };
        auto debit { *trans->debit };
        auto credit { *trans->credit };
        emit SUpdateOneTotal(node_id_, -debit, -credit, -ratio * debit, -ratio * credit);

        ratio = *trans->related_ratio;
        debit = *trans->related_debit;
        credit = *trans->related_credit;
        emit SUpdateOneTotal(*trans->related_node, -debit, -credit, -ratio * debit, -ratio * credit);

        int trans_id { *trans->id };
        emit SRemoveOne(info_.section, related_node_id, trans_id);
        AccumulateSubtotal(row, node_rule_);
        sql_->RemoveTransaction(trans_id);
    }

    ResourcePool<Trans>::Instance().Recycle(trans);
    return true;
}

bool TableModel::InsertTrans(const QModelIndex& parent)
{
    // just register trans in this function
    // while set related node in setData function, register transaction to sql_'s transaction_hash_
    auto row { trans_list_.size() };
    auto trans { sql_->AllocateTrans() };

    *trans->node = node_id_;

    beginInsertRows(parent, row, row);
    trans_list_.emplaceBack(trans);
    endInsertRows();

    return true;
}

void TableModel::UpdateAllState(Check state)
{
    for (auto& trans : trans_list_) {
        switch (state) {
        case Check::kAll:
            *trans->state = true;
            break;
        case Check::kNone:
            *trans->state = false;
            break;
        case Check::kReverse:
            *trans->state = !*trans->state;
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

bool TableModel::UpdateDebit(Trans* trans, double value)
{
    double debit { *trans->debit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(debit - value) < tolerance)
        return false;

    double credit { *trans->credit };
    double ratio { *trans->ratio };

    double abs { qAbs(value - credit) };
    *trans->debit = (value > credit) ? abs : 0;
    *trans->credit = (value <= credit) ? abs : 0;

    double related_debit { *trans->related_debit };
    double related_credit { *trans->related_credit };
    double related_ratio { *trans->related_ratio };

    *trans->related_debit = (*trans->credit) * ratio / related_ratio;
    *trans->related_credit = (*trans->debit) * ratio / related_ratio;

    if (*trans->related_node == 0)
        return false;

    auto initial_debit_diff { *trans->debit - debit };
    auto initial_credit_diff { *trans->credit - credit };
    emit SUpdateOneTotal(*trans->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto related_initial_debit_diff { *trans->related_debit - related_debit };
    auto related_initial_credit_diff { *trans->related_credit - related_credit };
    emit SUpdateOneTotal(*trans->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
        related_initial_credit_diff * related_ratio);

    return true;
}

bool TableModel::UpdateCredit(Trans* trans, double value)
{
    double credit { *trans->credit };
    const double tolerance { std::pow(10, -section_rule_.value_decimal - 2) };

    if (std::abs(credit - value) < tolerance)
        return false;

    double debit { *trans->debit };
    double ratio { *trans->ratio };

    double abs { qAbs(value - debit) };
    *trans->debit = (value > debit) ? 0 : abs;
    *trans->credit = (value <= debit) ? 0 : abs;

    double related_debit { *trans->related_debit };
    double related_credit { *trans->related_credit };
    double related_ratio { *trans->related_ratio };

    *trans->related_debit = (*trans->credit) * ratio / related_ratio;
    *trans->related_credit = (*trans->debit) * ratio / related_ratio;

    if (*trans->related_node == 0)
        return false;

    auto initial_debit_diff { *trans->debit - debit };
    auto initial_credit_diff { *trans->credit - credit };
    emit SUpdateOneTotal(*trans->node, initial_debit_diff, initial_credit_diff, initial_debit_diff * ratio, initial_credit_diff * ratio);

    auto related_initial_debit_diff { *trans->related_debit - related_debit };
    auto related_initial_credit_diff { *trans->related_credit - related_credit };
    emit SUpdateOneTotal(*trans->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
        related_initial_credit_diff * related_ratio);

    return true;
}

bool TableModel::UpdateRatio(Trans* trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_.ratio_decimal - 2) };
    double ratio { *trans->ratio };

    if (std::abs(ratio - value) < tolerance || value <= 0)
        return false;

    auto result { value - ratio };
    auto proportion { value / *trans->ratio };

    *trans->ratio = value;

    double related_debit { *trans->related_debit };
    double related_credit { *trans->related_credit };
    double related_ratio { *trans->related_ratio };

    *trans->related_debit *= proportion;
    *trans->related_credit *= proportion;

    if (*trans->related_node == 0)
        return false;

    emit SUpdateOneTotal(*trans->node, 0, 0, *trans->debit * result, *trans->credit * result);

    auto related_initial_debit_diff { *trans->related_debit - related_debit };
    auto related_initial_credit_diff { *trans->related_credit - related_credit };
    emit SUpdateOneTotal(*trans->related_node, related_initial_debit_diff, related_initial_credit_diff, related_initial_debit_diff * related_ratio,
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
    return trans_list_.size();
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

    auto trans { trans_list_.at(index.row()) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
        return *trans->id;
    case TableEnum::kDateTime:
        return *trans->date_time;
    case TableEnum::kCode:
        return *trans->code;
    case TableEnum::kRatio:
        return *trans->ratio;
    case TableEnum::kDescription:
        return *trans->description;
    case TableEnum::kRelatedNode:
        return *trans->related_node == 0 ? QVariant() : *trans->related_node;
    case TableEnum::kState:
        return *trans->state;
    case TableEnum::kDocument:
        return trans->document->isEmpty() ? QVariant() : QString::number(trans->document->size());
    case TableEnum::kDebit:
        return *trans->debit == 0 ? QVariant() : *trans->debit;
    case TableEnum::kCredit:
        return *trans->credit == 0 ? QVariant() : *trans->credit;
    case TableEnum::kSubtotal:
        return trans->subtotal;
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

    auto trans { trans_list_.at(kRow) };
    int old_related_node { *trans->related_node };

    bool rel_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool rat_changed { false };

    switch (kColumn) {
    case TableEnum::kDateTime:
        UpdateDateTime(trans, value.toString());
        break;
    case TableEnum::kCode:
        UpdateCode(trans, value.toString());
        break;
    case TableEnum::kState:
        UpdateOneState(trans, value.toBool());
        break;
    case TableEnum::kDescription:
        UpdateDescription(trans, value.toString());
        break;
    case TableEnum::kRatio:
        rat_changed = UpdateRatio(trans, value.toDouble());
        break;
    case TableEnum::kRelatedNode:
        rel_changed = UpdateRelatedNode(trans, value.toInt());
        break;
    case TableEnum::kDebit:
        deb_changed = UpdateDebit(trans, value.toDouble());
        break;
    case TableEnum::kCredit:
        cre_changed = UpdateCredit(trans, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_related_node == 0) {
        if (rel_changed) {
            sql_->InsertTrans(trans);
            AccumulateSubtotal(kRow, node_rule_);
            emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
            emit SAppendOne(info_.section, trans);

            auto ratio { *trans->ratio };
            auto debit { *trans->debit };
            auto credit { *trans->credit };
            emit SUpdateOneTotal(node_id_, debit, credit, ratio * debit, ratio * credit);

            ratio = *trans->related_ratio;
            debit = *trans->related_debit;
            credit = *trans->related_credit;
            emit SUpdateOneTotal(*trans->related_node, debit, credit, ratio * debit, ratio * credit);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->UpdateTransaction(*trans->id);
        emit SSearch();
        emit SUpdateBalance(info_.section, old_related_node, *trans->id);
    }

    if (deb_changed || cre_changed) {
        AccumulateSubtotal(kRow, node_rule_);
        emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
    }

    if (rel_changed) {
        sql_->UpdateTransaction(*trans->id);
        emit SMoveMulti(info_.section, old_related_node, *trans->related_node, QList<int> { *trans->id });

        auto ratio { *trans->related_ratio };
        auto debit { *trans->related_debit };
        auto credit { *trans->related_credit };
        emit SUpdateOneTotal(*trans->related_node, debit, credit, ratio * debit, ratio * credit);
        emit SUpdateOneTotal(old_related_node, -debit, -credit, -ratio * debit, -ratio * credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

void TableModel::sort(int column, Qt::SortOrder order)
{ // ignore subtotal column
    if (column <= -1 || column >= info_.part_table_header.size() - 1)
        return;

    auto Compare = [column, order](Trans* lhs, Trans* rhs) -> bool {
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
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();

    AccumulateSubtotal(0, node_rule_);
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

    for (const auto& trans : trans_list_) {
        if (*trans->related_node == node_id) {
            return row;
        }
        ++row;
    }
    return -1;
}

QModelIndex TableModel::GetIndex(int trans_id) const
{
    int row { 0 };

    for (const auto& trans : trans_list_) {
        if (*trans->id == trans_id) {
            return index(row, 0);
        }
        ++row;
    }
    return QModelIndex();
}

QStringList* TableModel::GetDocumentPointer(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= trans_list_.size()) {
        qWarning() << "Invalid QModelIndex provided.";
        return nullptr;
    }

    auto trans { trans_list_[index.row()] };

    if (!trans || !trans->document) {
        qWarning() << "Null pointer encountered in trans_list_ or document.";
        return nullptr;
    }

    return trans->document;
}

void TableModel::AccumulateSubtotal(int start, bool node_rule)
{
    if (start <= -1 || start >= trans_list_.size() || trans_list_.isEmpty())
        return;

    double previous_subtotal { start >= 1 ? trans_list_.at(start - 1)->subtotal : 0.0 };

    std::accumulate(trans_list_.begin() + start, trans_list_.end(), previous_subtotal, [&](double current_subtotal, Trans* trans) {
        trans->subtotal = Balance(node_rule, *trans->debit, *trans->credit) + current_subtotal;
        return trans->subtotal;
    });
}

bool TableModel::UpdateDateTime(Trans* trans, CString& new_value, CString& field) { return UpdateField(trans, new_value, field, &Trans::date_time); }

bool TableModel::UpdateDescription(Trans* trans, CString& new_value, CString& field)
{
    return UpdateField(trans, new_value, field, &Trans::description, [this]() { emit SSearch(); });
}

bool TableModel::UpdateCode(Trans* trans, CString& new_value, CString& field) { return UpdateField(trans, new_value, field, &Trans::code); }

bool TableModel::UpdateOneState(Trans* trans, bool new_value, CString& field) { return UpdateField(trans, new_value, field, &Trans::state); }

bool TableModel::UpdateRelatedNode(Trans* trans, int value)
{
    if (*trans->related_node == value)
        return false;

    *trans->related_node = value;
    return true;
}

bool TableModel::RemoveMulti(const QList<int>& trans_id_list)
{
    int min_row {};
    int trans_id {};

    for (int i = 0; i != trans_list_.size(); ++i) {
        trans_id = *trans_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            ResourcePool<Trans>::Instance().Recycle(trans_list_.takeAt(i));
            endRemoveRows();

            if (min_row == 0)
                min_row = i;

            --i;
        }
    }

    AccumulateSubtotal(min_row, node_rule_);
    return true;
}

bool TableModel::InsertMulti(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_list_.size() };
    TransList trans_list {};

    sql_->BuildTransList(trans_list, node_id, trans_id_list);
    beginInsertRows(QModelIndex(), row, row + trans_list.size() - 1);
    trans_list_.append(trans_list);
    endInsertRows();

    AccumulateSubtotal(row, node_rule_);
    return true;
}
