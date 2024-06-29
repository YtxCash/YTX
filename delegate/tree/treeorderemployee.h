#ifndef TREEORDEREMPLOYEE_H
#define TREEORDEREMPLOYEE_H

#include <QStyledItemDelegate>

#include "component/using.h"

class TreeOrderEmployee : public QStyledItemDelegate {
public:
    TreeOrderEmployee(CStringHash* branch_path, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CStringHash* branch_path_ {};
};

#endif // TREEORDEREMPLOYEE_H
