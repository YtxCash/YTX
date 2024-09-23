#ifndef TABLEMODELTASK_H
#define TABLEMODELTASK_H

#include "tablemodel.h"

class TableModelTask final : public TableModel {
    Q_OBJECT

public:
    TableModelTask(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    ~TableModelTask() override = default;

private:
    bool UpdateDebit(TransShadow* trans_shadow, double value) override;
    bool UpdateCredit(TransShadow* trans_shadow, double value) override;
    bool UpdateRatio(TransShadow* trans_shadow, double value) override;
};

#endif // TABLEMODELTASK_H