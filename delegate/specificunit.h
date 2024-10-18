#ifndef SPECIFICUNIT_H
#define SPECIFICUNIT_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class SpecificUnit : public StyledItemDelegate {
public:
    SpecificUnit(const TreeModel* tree_model, int unit, bool skip_branch, UnitFilterMode unit_filter_mode, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const TreeModel* tree_model_ {};
    const UnitFilterMode unit_filter_mode_ {};
    const int unit_ {};
    const bool skip_branch_ {};
};

#endif // SPECIFICUNIT_H
