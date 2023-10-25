#ifndef TABLEMODELPRODUCT_H
#define TABLEMODELPRODUCT_H

#include "tablemodel.h"

class TableModelProduct final : public TableModel {
    Q_OBJECT

public:
    TableModelProduct(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    ~TableModelProduct() override = default;

private:
    bool UpdateDebit(Trans* trans, double value) override;
    bool UpdateCredit(Trans* trans, double value) override;
    bool UpdateRatio(Trans* trans, double value) override;
};

#endif // TABLEMODELPRODUCT_H
