#ifndef MAINWINDOWSQLITE_H
#define MAINWINDOWSQLITE_H

#include <QSqlDatabase>

#include "component/enumclass.h"
#include "component/settings.h"
#include "component/using.h"

class MainwindowSqlite {
public:
    MainwindowSqlite() = default;
    explicit MainwindowSqlite(Section section);

    void QuerySettings(Settings& settings, Section section);
    void UpdateSettings(CSettings& settings, Section section);
    void NewFile(CString& file_path);

private:
    QString NodeFinanceTask(CString& table_name);
    QString NodeStakeholder(CString& table_name);
    QString NodeProduct(CString& table_name);
    QString NodeOrder(CString& table_name);

    QString Path(CString& table_name);

    QString TransactionFinance(CString& table_name);
    QString TransactionOrder(CString& table_name);
    QString TransactionStakeholder(CString& table_name);
    QString TransactionTask(CString& table_name);
    QString TransactionProduct(CString& table_name);

private:
    QSqlDatabase* db_ {};
};

#endif // MAINWINDOWSQLITE_H
