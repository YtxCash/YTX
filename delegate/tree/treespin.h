#ifndef TREESPIN_H
#define TREESPIN_H

#include "delegate/styleditemdelegate.h"

class TreeSpin final : public StyledItemDelegate {
public:
    TreeSpin(int min, int max, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int max_ {};
    int min_ {};
};

#endif // TREESPIN_H
