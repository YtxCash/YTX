#ifndef SQLITEFINANCE_H
#define SQLITEFINANCE_H

#include "sqlite.h"

class SqliteFinance final : public Sqlite {
public:
    SqliteFinance(CInfo& info, QObject* parent = nullptr);

protected:
    QString BuildTreeQS() const override;
    QString InsertNodeQS() const override;
    QString RemoveNodeSecondQS() const override;
    QString InternalReferenceQS() const override;
    QString ExternalReferenceQS() const override { return QString(); }
    QString LeafTotalQS() const override;

    QString BuildTransShadowListQS() const override;
    QString InsertTransShadowQS() const override;
    QString BuildTransShadowListRangQS(CString& in_list) const override;
    QString RReplaceNodeQS() const override;
    QString RUpdateProductReferenceQS() const override { return QString(); }
    QString RUpdateStakeholderReferenceQS() const override { return {}; }
};

#endif // SQLITEFINANCE_H
