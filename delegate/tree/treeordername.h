#ifndef TREEORDERNAME_H
#define TREEORDERNAME_H

#include <QStyledItemDelegate>

#include "component/using.h"

class TreeOrderName : public QStyledItemDelegate {
public:
    TreeOrderName(CStringHash* branch_path, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CStringHash* branch_path_ {};
};

#endif // TREEORDERNAME_H
