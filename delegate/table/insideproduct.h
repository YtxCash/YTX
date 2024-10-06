#ifndef INSIDEPRODUCT_H
#define INSIDEPRODUCT_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class InsideProduct final : public StyledItemDelegate {
public:
    InsideProduct(const TreeModel* model, int exclude_unit, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int exclude_unit_ {};
    const TreeModel* model_ {};
};

#endif // INSIDEPRODUCT_H
