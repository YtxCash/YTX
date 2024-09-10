#ifndef TABLECOMBO_H
#define TABLECOMBO_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/abstracttreemodel.h"

class TableCombo final : public StyledItemDelegate {
public:
    TableCombo(const AbstractTreeModel& model, int exclude, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int exclude_ {};
    mutable int last_insert_ {};
    const AbstractTreeModel& model_;
};

#endif // TABLECOMBO_H
