#ifndef ORDEREMPLOYEE_H
#define ORDEREMPLOYEE_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/abstracttreemodel.h"

class OrderEmployee : public StyledItemDelegate {
public:
    OrderEmployee(const AbstractTreeModel& stakeholder_tree_model, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const AbstractTreeModel& stakeholder_tree_model_;
};

#endif // ORDEREMPLOYEE_H
