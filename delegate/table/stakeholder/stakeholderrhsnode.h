#ifndef STAKEHOLDERRHSNODE_H
#define STAKEHOLDERRHSNODE_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class StakeholderRhsNode final : public StyledItemDelegate {
public:
    StakeholderRhsNode(const TreeModel* model, int exclude_unit, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int exclude_unit_ {};
    mutable int last_insert_ {};
    const TreeModel* model_ {};
};

#endif // STAKEHOLDERRHSNODE_H
