#ifndef ORDERNAMER_H
#define ORDERNAMER_H

#include <QStyledItemDelegate>

#include "component/using.h"

class OrderName : public QStyledItemDelegate {
public:
    OrderName(CStringHash* stakeholder_branch_path, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CStringHash* stakeholder_branch_path_ {};
};

#endif // ORDERNAMER_H
