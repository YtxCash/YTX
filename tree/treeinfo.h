#ifndef TREEINFO_H
#define TREEINFO_H

#include <QString>

struct TreeInfo {
    QString node_table {};
    QString node_path_table {};
    QString node_transaction_table {};

    TreeInfo() = default;
    TreeInfo(const QString& node_table, const QString& node_path_table, const QString& node_transaction_table)
        : node_table { node_table }
        , node_path_table { node_path_table }
        , node_transaction_table { node_transaction_table }
    {
    }
};

#endif // TREEINFO_H
