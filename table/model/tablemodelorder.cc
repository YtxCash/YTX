#include "tablemodelorder.h"

TableModelOrder::TableModelOrder(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
    AccumulateSubtotal(0, node_rule);
}

QVariant TableModelOrder::data(const QModelIndex& index, int role) const
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

bool TableModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto trans { trans_list_.at(kRow) };
    int old_related_node { *trans->related_node };

    bool tra_changed { false };
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
        tra_changed = UpdateRelatedNode(trans, value.toInt());
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
        if (tra_changed) {
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

    if (tra_changed) {
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

void TableModelOrder::sort(int column, Qt::SortOrder order)
{
    // ignore subtotal column
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

Qt::ItemFlags TableModelOrder::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags { QAbstractItemModel::flags(index) };
    const TableEnum kColumn { index.column() };

    switch (kColumn) {
    case TableEnum::kID:
    case TableEnum::kSubtotal:
    case TableEnum::kDocument:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
