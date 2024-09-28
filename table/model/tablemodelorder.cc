#include "tablemodelorder.h"

#include "component/constvalue.h"

TableModelOrder::TableModelOrder(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent)
    : TableModel { sql, node_rule, node_id, info, section_rule, parent }
{
}

QVariant TableModelOrder::data(const QModelIndex& index, int role) const
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

bool TableModelOrder::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const TableEnum kColumn { index.column() };
    const int kRow { index.row() };

    auto trans_shadow { trans_shadow_list_.at(kRow) };
    int old_related_node { *trans_shadow->related_node };

    bool tra_changed { false };
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
        tra_changed = UpdateRelatedNode(trans_shadow, value.toInt());
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
        if (tra_changed) {
            sql_->InsertTransShadow(trans_shadow);
            AccumulateSubtotal(kRow, node_rule_);
            emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
            emit SAppendOne(info_.section, trans_shadow);

            auto ratio { *trans_shadow->ratio };
            auto debit { *trans_shadow->debit };
            auto credit { *trans_shadow->credit };
            emit SUpdateOneTotal(node_id_, debit, credit, ratio * debit, ratio * credit);

            ratio = *trans_shadow->related_ratio;
            debit = *trans_shadow->related_debit;
            credit = *trans_shadow->related_credit;
            emit SUpdateOneTotal(*trans_shadow->related_node, debit, credit, ratio * debit, ratio * credit);
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
        AccumulateSubtotal(kRow, node_rule_);
        emit SResizeColumnToContents(std::to_underlying(TableEnum::kSubtotal));
    }

    if (tra_changed) {
        sql_->UpdateTrans(*trans_shadow->id);
        emit SMoveMultiTrans(info_.section, old_related_node, *trans_shadow->related_node, QList<int> { *trans_shadow->id });

        auto ratio { *trans_shadow->related_ratio };
        auto debit { *trans_shadow->related_debit };
        auto credit { *trans_shadow->related_credit };
        emit SUpdateOneTotal(*trans_shadow->related_node, debit, credit, ratio * debit, ratio * credit);
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
    case TableEnum::kState:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
