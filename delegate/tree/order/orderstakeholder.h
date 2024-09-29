#ifndef ORDERSTAKEHOLDER_H
#define ORDERSTAKEHOLDER_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class OrderStakeholder : public StyledItemDelegate {
public:
    OrderStakeholder(const TreeModel* stakeholder_tree_model, int unit, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const TreeModel* stakeholder_tree_model_ {};
    int unit_ {};
};

#endif // ORDERSTAKEHOLDER_H
