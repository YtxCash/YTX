#include "treemodeltask.h"

TreeModelTask::TreeModelTask(SPSqlite sql, CInfo& info, int base_unit, CTableHash& table_hash, CString& separator, QObject* parent)
    : TreeModel(sql, info, base_unit, table_hash, separator, parent)
{
    ConstructTree();
}

QVariant TreeModelTask::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node { GetNodeByIndex(index) };
    if (node->id == -1)
        return QVariant();

    const TreeEnum kColumn { index.column() };

    switch (kColumn) {
    case TreeEnum::kName:
        return node->name;
    case TreeEnum::kID:
        return node->id;
    case TreeEnum::kCode:
        return node->code;
    case TreeEnum::kDescription:
        return node->description;
    case TreeEnum::kNote:
        return node->note;
    case TreeEnum::kRule:
        return node->rule;
    case TreeEnum::kBranch:
        return node->branch;
    case TreeEnum::kUnit:
        return node->unit;
    case TreeEnum::kInitialTotal:
        return node->initial_total;
    case TreeEnum::kFinalTotal:
        return node->final_total;
    default:
        return QVariant();
    }
}
