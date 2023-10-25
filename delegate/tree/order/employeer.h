#ifndef EMPLOYEER_H
#define EMPLOYEER_H

#include <QStyledItemDelegate>

#include "tree/model/treemodel.h"

class EmployeeR : public QStyledItemDelegate {
public:
    EmployeeR(const TreeModel& model, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const TreeModel& model_;
};

#endif // EMPLOYEER_H
