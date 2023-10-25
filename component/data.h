#ifndef DATA_H
#define DATA_H

#include "database/searchsqlite.h"
#include "database/sqlite/sqlite.h"
#include "enumclass.h"
#include "tree/model/treemodel.h"
#include "widget/abstracttreewidget.h"

struct Tab {
    Section section {};
    int node_id {};

    // Equality operator overload to compare two Tab structs
    bool operator==(const Tab& other) const noexcept { return std::tie(section, node_id) == std::tie(other.section, other.node_id); }

    // Inequality operator overload to compare two Tab structs
    bool operator!=(const Tab& other) const noexcept { return !(*this == other); }
};

struct Tree {
    AbstractTreeWidget* widget {};
    TreeModel* model {};
};

struct SectionData {
    Tab last_tab {};
    Info info {};
    QSharedPointer<Sqlite> sql {};
    QSharedPointer<SearchSqlite> search_sql {};
};

#endif // DATA_H
