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
    QString NodeFinance();
    QString NodeStakeholder();
    QString NodeProduct();
    QString NodeTask();
    QString NodeSales();
    QString NodePurchase();

    QString Path(CString& table_name);

    QString TransactionFinance();
    QString TransactionTask();
    QString TransactionProduct();
    QString TransactionStakeholder();
    QString TransactionSales();
    QString TransactionPurchase();

private:
    QSqlDatabase* db_ {};
};

#endif // MAINWINDOWSQLITE_H
