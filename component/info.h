#ifndef INFO_H
#define INFO_H

#include <QHash>
#include <QStringList>
#include <tuple>

#include "enumclass.h"

struct Info {
    Section section {};

    QString node {}; // SQL database node table name, also used as QSettings section name, be carefull with it
    QString path {}; // SQL database node_path table name
    QString transaction {}; // SQL database node_transaction table name

    QStringList tree_header {};
    QStringList table_header {};

    QStringList search_trans_header {};
    QStringList search_node_header {};

    QHash<int, QString> unit_hash {};
    QHash<int, QString> unit_symbol_hash {};
    QHash<int, QString> rule_hash { { 0, "DICD" }, { 1, "DDCI" } };
};

struct Tab {
    Section section {};
    int node_id {};

    // Equality operator overload to compare two Tab structs
    bool operator==(const Tab& other) const noexcept { return std::tie(section, node_id) == std::tie(other.section, other.node_id); }

    // Inequality operator overload to compare two Tab structs
    bool operator!=(const Tab& other) const noexcept { return !(*this == other); }
};

using CInfo = const Info;
using CTab = const Tab;

#endif // INFO_H
