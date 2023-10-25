#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QDate>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Table {
enum Column {
    kID,
    kPostDate,
    kDescription,
    kTransfer,
    kStatus,
    kDocument,
    kDebit,
    kCredit,
    kBalance,
};
}
QT_END_NAMESPACE

struct Transaction {
    int id { 0 };
    QDate post_date { QDate::currentDate() };
    QString description {};
    int transfer { 0 };
    bool status { false };
    QStringList document {};
    double debit { 0.0 };
    double credit { 0.0 };
    double balance { 0.0 };

    void ResetToDefault()
    {
        id = 0;
        post_date = QDate::currentDate();
        description.clear();
        transfer = 0;
        status = false;
        document.clear();
        debit = 0.0;
        credit = 0.0;
        balance = 0.0;
    };
};

#endif // TRANSACTION_H
