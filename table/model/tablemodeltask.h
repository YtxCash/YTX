#ifndef TABLEMODELTASK_H
#define TABLEMODELTASK_H

#include "tablemodel.h"

class TableModelTask final : public TableModel {
    Q_OBJECT

public:
    TableModelTask(SPSqlite sql, bool node_rule, const int node_id, CInfo& info, CSectionRule& section_rule, QObject* parent = nullptr);
    ~TableModelTask() override = default;

private:
    bool UpdateDebit(Trans* trans, double value) override;
    bool UpdateCredit(Trans* trans, double value) override;
    bool UpdateRatio(Trans* trans, double value) override;
};

#endif // TABLEMODELTASK_H
