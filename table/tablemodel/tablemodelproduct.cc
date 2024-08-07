#include "tablemodelproduct.h"

#include "component/constvalue.h"
#include "global/transpool.h"

TableModelProduct::TableModelProduct(const Info* info, const SectionRule* section_rule, QSharedPointer<Sql> sql, int node_id, bool node_rule, QObject* parent)
    : AbstractTableModel { parent }
    , info_ { info }
    , section_rule_ { section_rule }
    , sql_ { sql }
    , node_id_ { node_id }
    , node_rule_ { node_rule }
{
    trans_list_ = sql_->TransList(node_id);
    AccumulateBalance(trans_list_, 0, node_rule);
}

TableModelProduct::~TableModelProduct() { RecycleTrans(trans_list_); }

void TableModelProduct::RNodeRule(int node_id, bool node_rule)
{
    if (node_id_ != node_id || node_rule_ == node_rule)
        return;

    for (auto& trans : trans_list_)
        trans->balance = -trans->balance;

    node_rule_ = node_rule;
}

void TableModelProduct::RAppendOne(CSPCTrans& trans)
{
    if (node_id_ != *trans->related_node)
        return;

    auto new_trans { TransPool::Instance().Allocate() };
    new_trans->date_time = trans->date_time;
    new_trans->id = trans->id;
    new_trans->description = trans->description;
    new_trans->code = trans->code;
    new_trans->document = trans->document;
    new_trans->state = trans->state;
    new_trans->transport = trans->transport;
    new_trans->location = trans->location;

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

    double previous_balance { row >= 1 ? trans_list_.at(row - 1)->balance : 0.0 };
    new_trans->balance = Balance(node_rule_, *new_trans->debit, *new_trans->credit) + previous_balance;
}

void TableModelProduct::RRetrieveOne(CSPTrans& trans)
{
    auto row { trans_list_.size() };

    beginInsertRows(QModelIndex(), row, row);
    trans_list_.emplaceBack(trans);
    endInsertRows();

    double previous_balance { row >= 1 ? trans_list_.at(row - 1)->balance : 0.0 };
    trans->balance = Balance(node_rule_, *trans->debit, *trans->credit) + previous_balance;
}

bool TableModelProduct::AppendOne(const QModelIndex& parent)
{
    // just register trans this function
    // while set related node in setData function, register related transaction to sql_'s transaction_hash_
    auto row { trans_list_.size() };
    auto trans { sql_->AllocateTrans() };

    *trans->node = node_id_;

    beginInsertRows(parent, row, row);
    trans_list_.emplaceBack(trans);
    endInsertRows();

    return true;
}

void TableModelProduct::RUpdateBalance(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto index { TransIndex(trans_id) };
    if (index.isValid())
        AccumulateBalance(trans_list_, index.row(), node_rule_);
}

void TableModelProduct::RRemoveOne(int node_id, int trans_id)
{
    if (node_id_ != node_id)
        return;

    auto idx { TransIndex(trans_id) };
    if (!idx.isValid())
        return;

    int row { idx.row() };
    beginRemoveRows(QModelIndex(), row, row);
    TransPool::Instance().Recycle(trans_list_.takeAt(row));
    endRemoveRows();

    AccumulateBalance(trans_list_, row, node_rule_);
}

bool TableModelProduct::DeleteOne(int row, const QModelIndex& parent)
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
        emit SUpdateOneTotal(node_id_, -ratio * debit, -ratio * credit, -debit, -credit);

        auto t_ratio { *trans->related_ratio };
        auto t_debit { *trans->related_debit };
        auto t_credit { *trans->related_credit };
        emit SUpdateOneTotal(*trans->related_node, -t_ratio * t_debit, -t_ratio * t_credit, -t_debit, -t_credit);

        int trans_id { *trans->id };
        emit SRemoveOne(info_->section, related_node_id, trans_id);
        AccumulateBalance(trans_list_, row, node_rule_);
        sql_->Delete(trans_id);
    }

    TransPool::Instance().Recycle(trans);
    return true;
}

void TableModelProduct::RMoveMulti(int old_node_id, int new_node_id, const QList<int>& trans_id_list)
{
    if (node_id_ == old_node_id)
        RemoveMulti(trans_id_list);

    if (node_id_ == new_node_id)
        AppendMulti(new_node_id, trans_id_list);
}

void TableModelProduct::RRemoveMulti(const QMultiHash<int, int>& node_trans)
{
    if (!node_trans.contains(node_id_))
        return;

    auto trans_list { node_trans.values(node_id_) };
    RemoveMulti(trans_list);
}

bool TableModelProduct::RemoveMulti(const QList<int>& trans_id_list)
{
    int min_row {};
    int trans_id {};

    for (int i = 0; i != trans_list_.size(); ++i) {
        trans_id = *trans_list_.at(i)->id;

        if (trans_id_list.contains(trans_id)) {
            beginRemoveRows(QModelIndex(), i, i);
            TransPool::Instance().Recycle(trans_list_.takeAt(i));
            endRemoveRows();

            if (min_row == 0)
                min_row = i;

            --i;
        }
    }

    AccumulateBalance(trans_list_, min_row, node_rule_);
    return true;
}

QModelIndex TableModelProduct::TransIndex(int trans_id) const
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

void TableModelProduct::UpdateState(Check state)
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
        sql_->Update("state", true, state);
        break;
    case Check::kNone:
        sql_->Update("state", false, state);
        break;
    case Check::kReverse:
        sql_->Update("state", true, state);
        break;
    default:
        break;
    }

    int column { std::to_underlying(PartTableColumn::kState) };
    emit dataChanged(index(0, column), index(rowCount() - 1, column));
}

int TableModelProduct::NodeRow(int node_id) const
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

void TableModelProduct::RecycleTrans(SPTransList& list)
{
    for (auto& trans : list)
        TransPool::Instance().Recycle(trans);

    list.clear();
}

QModelIndex TableModelProduct::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TableModelProduct::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

int TableModelProduct::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return trans_list_.size();
}

int TableModelProduct::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return info_->part_table_header.size();
}

QVariant TableModelProduct::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto trans { trans_list_.at(index.row()) };
    const PartTableColumn kColumn { index.column() };

    switch (kColumn) {
    case PartTableColumn::kID:
        return *trans->id;
    case PartTableColumn::kDateTime:
        return *trans->date_time;
    case PartTableColumn::kCode:
        return *trans->code;
    case PartTableColumn::kRatio:
        return *trans->ratio;
    case PartTableColumn::kDescription:
        return *trans->description;
    case PartTableColumn::kRelatedNode:
        return *trans->related_node == 0 ? QVariant() : *trans->related_node;
    case PartTableColumn::kState:
        return *trans->state;
    case PartTableColumn::kTransport:
        return *trans->transport == 0 ? QVariant() : (*trans->transport == 1 ? tr("S %1").arg(trans->location->size() / 2) : tr("R"));
    case PartTableColumn::kDocument:
        return trans->document->isEmpty() ? QVariant() : QString::number(trans->document->size());
    case PartTableColumn::kDebit:
        return *trans->debit == 0 ? QVariant() : *trans->debit;
    case PartTableColumn::kCredit:
        return *trans->credit == 0 ? QVariant() : *trans->credit;
    case PartTableColumn::kRemainder:
        return trans->balance;
    default:
        return QVariant();
    }
}

bool TableModelProduct::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const PartTableColumn kColumn { index.column() };
    const int kRow { index.row() };

    auto trans { trans_list_.at(kRow) };
    int old_related_node { *trans->related_node };

    bool tra_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool rat_changed { false };

    switch (kColumn) {
    case PartTableColumn::kDateTime:
        UpdateDateTime(trans, value.toString());
        break;
    case PartTableColumn::kCode:
        UpdateCode(trans, value.toString());
        break;
    case PartTableColumn::kState:
        UpdateState(trans, value.toBool());
        break;
    case PartTableColumn::kDescription:
        UpdateDescription(trans, value.toString());
        break;
    case PartTableColumn::kRatio:
        rat_changed = UpdateUnitCost(trans, value.toDouble());
        break;
    case PartTableColumn::kRelatedNode:
        tra_changed = UpdateRelatedNode(trans, value.toInt());
        break;
    case PartTableColumn::kDebit:
        deb_changed = UpdateDebit(trans, value.toDouble());
        break;
    case PartTableColumn::kCredit:
        cre_changed = UpdateCredit(trans, value.toDouble());
        break;
    default:
        return false;
    }

    if (old_related_node == 0) {
        if (tra_changed) {
            sql_->Insert(trans);
            AccumulateBalance(trans_list_, kRow, node_rule_);
            emit SResizeColumnToContents(std::to_underlying(PartTableColumn::kRemainder));
            emit SAppendOne(info_->section, trans);

            auto debit { *trans->debit };
            auto credit { *trans->credit };
            emit SUpdateOneTotal(*trans->node, debit, credit, debit, credit);

            debit = *trans->related_debit;
            credit = *trans->related_credit;
            emit SUpdateOneTotal(*trans->related_node, debit, credit, debit, credit);
        }

        emit SResizeColumnToContents(index.column());
        return true;
    }

    if (deb_changed || cre_changed || rat_changed) {
        sql_->Update(*trans->id);
        emit SSearch();
        emit SUpdateBalance(info_->section, old_related_node, *trans->id);
    }

    if (deb_changed || cre_changed) {
        AccumulateBalance(trans_list_, kRow, node_rule_);
        emit SResizeColumnToContents(std::to_underlying(PartTableColumn::kRemainder));
    }

    if (tra_changed) {
        sql_->Update(*trans->id);
        emit SMoveMulti(info_->section, old_related_node, *trans->related_node, QList<int> { *trans->id });

        auto debit { *trans->related_debit };
        auto credit { *trans->related_credit };
        emit SUpdateOneTotal(*trans->related_node, debit, credit, debit, credit);
        emit SUpdateOneTotal(old_related_node, -debit, -credit, -debit, -credit);
    }

    emit SResizeColumnToContents(index.column());
    return true;
}

bool TableModelProduct::UpdateDateTime(SPTrans& trans, CString& value)
{
    if (*trans->date_time == value)
        return false;

    *trans->date_time = value;

    if (*trans->related_node != 0)
        sql_->Update(info_->transaction, DATE_TIME, value, *trans->id);

    return true;
}

bool TableModelProduct::UpdateDescription(SPTrans& trans, CString& value)
{
    if (*trans->description == value)
        return false;

    *trans->description = value;

    if (*trans->related_node != 0) {
        sql_->Update(info_->transaction, DESCRIPTION, value, *trans->id);
        emit SSearch();
    }

    return true;
}

bool TableModelProduct::UpdateCode(SPTrans& trans, CString& value)
{
    if (*trans->code == value)
        return false;

    *trans->code = value;

    if (*trans->related_node != 0)
        sql_->Update(info_->transaction, CODE, value, *trans->id);

    return true;
}

bool TableModelProduct::UpdateState(SPTrans& trans, bool value)
{
    if (*trans->state == value)
        return false;

    *trans->state = value;

    if (*trans->related_node != 0)
        sql_->Update(info_->transaction, STATE, value, *trans->id);

    return true;
}

bool TableModelProduct::UpdateDebit(SPTrans& trans, double value)
{
    double debit { *trans->debit };
    const double tolerance { std::pow(10, -section_rule_->value_decimal - 2) };

    if (std::abs(debit - value) < tolerance)
        return false;

    double credit { *trans->credit };

    double abs { qAbs(value - credit) };
    *trans->debit = (value > credit) ? abs : 0;
    *trans->credit = (value <= credit) ? abs : 0;

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto unit_cost { *trans->ratio };
    auto quantity_debit_diff { *trans->debit - debit };
    auto quantity_credit_diff { *trans->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans->node, amount_debit_diff, amount_credit_diff, quantity_debit_diff, quantity_credit_diff);
    emit SUpdateOneTotal(*trans->related_node, amount_credit_diff, amount_debit_diff, quantity_credit_diff, quantity_debit_diff);

    return true;
}

bool TableModelProduct::UpdateCredit(SPTrans& trans, double value)
{
    double credit { *trans->credit };
    const double tolerance { std::pow(10, -section_rule_->value_decimal - 2) };

    if (std::abs(credit - value) < tolerance)
        return false;

    double debit { *trans->debit };

    double abs { qAbs(value - debit) };
    *trans->debit = (value > debit) ? 0 : abs;
    *trans->credit = (value <= debit) ? 0 : abs;

    *trans->related_debit = *trans->credit;
    *trans->related_credit = *trans->debit;

    if (*trans->related_node == 0)
        return false;

    auto unit_cost { *trans->ratio };
    auto quantity_debit_diff { *trans->debit - debit };
    auto quantity_credit_diff { *trans->credit - credit };
    auto amount_debit_diff { quantity_debit_diff * unit_cost };
    auto amount_credit_diff { quantity_credit_diff * unit_cost };

    emit SUpdateOneTotal(*trans->node, amount_debit_diff, amount_credit_diff, quantity_debit_diff, quantity_credit_diff);
    emit SUpdateOneTotal(*trans->related_node, amount_credit_diff, amount_debit_diff, quantity_credit_diff, quantity_debit_diff);

    return true;
}

bool TableModelProduct::UpdateUnitCost(SPTrans& trans, double value)
{
    const double tolerance { std::pow(10, -section_rule_->ratio_decimal - 2) };
    double ratio { *trans->ratio };

    if (std::abs(ratio - value) < tolerance || value < 0)
        return false;

    auto result { value - ratio };
    *trans->ratio = value;
    *trans->related_ratio = value;

    if (*trans->related_node == 0)
        return false;

    emit SUpdateOneTotal(*trans->node, *trans->debit * result, *trans->credit * result, 0, 0);
    emit SUpdateOneTotal(*trans->related_node, *trans->related_debit * result, *trans->related_credit * result, 0, 0);

    return true;
}

bool TableModelProduct::UpdateRelatedNode(SPTrans& trans, int value)
{
    if (*trans->related_node == value)
        return false;

    *trans->related_node = value;
    return true;
}

double TableModelProduct::Balance(bool node_rule, double debit, double credit) { return node_rule ? credit - debit : debit - credit; }

void TableModelProduct::AccumulateBalance(const SPTransList& list, int row, bool node_rule)
{
    if (row <= -1 || row >= list.size() || list.isEmpty())
        return;

    double previous_balance { row >= 1 ? list.at(row - 1)->balance : 0.0 };

    std::accumulate(list.begin() + row, list.end(), previous_balance, [&](double balance, SPTrans trans) {
        trans->balance = Balance(node_rule, *trans->debit, *trans->credit) + balance;
        return trans->balance;
    });
}

QVariant TableModelProduct::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return info_->part_table_header.at(section);

    return QVariant();
}

void TableModelProduct::sort(int column, Qt::SortOrder order)
{
    // ignore balance column
    if (column <= -1 || column >= info_->part_table_header.size() - 1)
        return;

    auto Compare = [column, order](SPTrans& lhs, SPTrans& rhs) -> bool {
        const PartTableColumn kColumn { column };

        switch (kColumn) {
        case PartTableColumn::kDateTime:
            return (order == Qt::AscendingOrder) ? (*lhs->date_time < *rhs->date_time) : (*lhs->date_time > *rhs->date_time);
        case PartTableColumn::kCode:
            return (order == Qt::AscendingOrder) ? (*lhs->code < *rhs->code) : (*lhs->code > *rhs->code);
        case PartTableColumn::kRatio:
            return (order == Qt::AscendingOrder) ? (*lhs->ratio < *rhs->ratio) : (*lhs->ratio > *rhs->ratio);
        case PartTableColumn::kDescription:
            return (order == Qt::AscendingOrder) ? (*lhs->description < *rhs->description) : (*lhs->description > *rhs->description);
        case PartTableColumn::kRelatedNode:
            return (order == Qt::AscendingOrder) ? (*lhs->related_node < *rhs->related_node) : (*lhs->related_node > *rhs->related_node);
        case PartTableColumn::kState:
            return (order == Qt::AscendingOrder) ? (*lhs->state < *rhs->state) : (*lhs->state > *rhs->state);
        case PartTableColumn::kTransport:
            return (order == Qt::AscendingOrder) ? (*lhs->transport < *rhs->transport) : (*lhs->transport > *rhs->transport);
        case PartTableColumn::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document->size() < rhs->document->size()) : (lhs->document->size() > rhs->document->size());
        case PartTableColumn::kDebit:
            return (order == Qt::AscendingOrder) ? (*lhs->debit < *rhs->debit) : (*lhs->debit > *rhs->debit);
        case PartTableColumn::kCredit:
            return (order == Qt::AscendingOrder) ? (*lhs->credit < *rhs->credit) : (*lhs->credit > *rhs->credit);
        default:
            return false;
        }
    };

    emit layoutAboutToBeChanged();
    std::sort(trans_list_.begin(), trans_list_.end(), Compare);
    emit layoutChanged();

    AccumulateBalance(trans_list_, 0, node_rule_);
}

bool TableModelProduct::AppendMulti(int node_id, const QList<int>& trans_id_list)
{
    auto row { trans_list_.size() };
    auto trans_list { sql_->TransList(node_id, trans_id_list) };

    beginInsertRows(QModelIndex(), row, row + trans_list.size() - 1);
    trans_list_.append(trans_list);
    endInsertRows();

    AccumulateBalance(trans_list_, row, node_rule_);
    return true;
}

Qt::ItemFlags TableModelProduct::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const PartTableColumn kColumn { index.column() };

    switch (kColumn) {
    case PartTableColumn::kID:
    case PartTableColumn::kRemainder:
    case PartTableColumn::kDocument:
    case PartTableColumn::kTransport:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
