#ifndef TABLEINFO_H
#define TABLEINFO_H

#include <QString>

struct TableInfo {
    TableInfo(int node_id, QString node_name, int decimal, bool node_rule, const QString& transaction_table)
        : node_id { node_id }
        , node_name { node_name }
        , decimal { decimal }
        , node_rule { node_rule }
        , transaction_table { transaction_table }
    {
    }

    int node_id { 0 };
    QString node_name {};
    int decimal { 0 };
    bool node_rule { false };
    QString transaction_table {};
};

#endif // TABLEINFO_H
