#ifndef CLASSPARAMS_H
#define CLASSPARAMS_H

#include <QStandardItemModel>

#include "component/settings.h"
#include "database/sqlite/sqlite.h"
#include "table/model/tablemodel.h"
#include "tree/model/treemodel.h"
#include "tree/node.h"

struct EditNodeParamsFPTS {
    Node* node {};
    QStandardItemModel* unit_model {};
    CString& parent_path {};
    CStringList& name_list {};
    bool type_enable {};
    bool unit_enable {};
};

struct EditNodeParamsOrder {
    NodeShadow* node_shadow {};
    Sqlite* sql {};
    TableModel* order_table {};
    TreeModel* stakeholder_tree {};
    CSettings* settings {};
    Section section {};
};

using CEditNodeParamsFPTS = const EditNodeParamsFPTS;
using CEditNodeParamsOrder = const EditNodeParamsOrder;

#endif // CLASSPARAMS_H
