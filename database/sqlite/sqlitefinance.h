#ifndef SQLITEFINANCE_H
#define SQLITEFINANCE_H

#include "sqlite.h"

class SqliteFinance final : public Sqlite {
public:
    SqliteFinance(CInfo& info, QObject* parent = nullptr);

protected:
    QString ReadNodeQS() const override;
    QString WriteNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString LeafTotalQS() const override;

    QString ReadTransQS() const override;
    QString WriteTransQS() const override;
    QString ReadTransRangeQS(CString& in_list) const override;
    QString RReplaceNodeQS() const override;
    QString UpdateTransQS() const override;
    QString SearchTransQS() const override;
};

#endif // SQLITEFINANCE_H
