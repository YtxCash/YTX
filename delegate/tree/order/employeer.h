#ifndef EMPLOYEER_H
#define EMPLOYEER_H

#include <QStyledItemDelegate>

#include "component/using.h"

class EmployeeR : public QStyledItemDelegate {
public:
    EmployeeR(CStringHash* stakeholder_branch_path, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CStringHash* stakeholder_branch_path_ {};
};

#endif // EMPLOYEER_H
