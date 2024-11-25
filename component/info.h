/*
 * Copyright (C) 2023 YtxCash
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef INFO_H
#define INFO_H

#include <QMap>
#include <QStandardItemModel>
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
    QStringList support_header {};

    QStringList search_trans_header {};
    QStringList search_node_header {};

    QMap<int, QString> unit_map {};
    QMap<int, QString> unit_symbol_map {};
    QMap<int, QString> rule_map {};
    QMap<int, QString> type_map {};

    QStandardItemModel* unit_model {};
    QStandardItemModel* rule_model {};
    QStandardItemModel* type_model {};
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
