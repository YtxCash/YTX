#ifndef SQLITETASK_H
#define SQLITETASK_H

#include "sqlite.h"

class SqliteTask final : public Sqlite {
public:
    SqliteTask(CInfo& info, QObject* parent = nullptr);

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
    QString UpdateTransQS() const override;

    void ReadTrans(Trans* trans, const QSqlQuery& query) override;
    void WriteTransShadow(TransShadow* trans_shadow, QSqlQuery& query) override;
    void UpdateTransBind(Trans* trans, QSqlQuery& query) override;
};

#endif // SQLITETASK_H
