#ifndef SPECIFICUNIT_H
#define SPECIFICUNIT_H

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class SpecificUnit : public StyledItemDelegate {
public:
    SpecificUnit(const TreeModel* tree_model, int specific_unit, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const TreeModel* tree_model_ {};
    int specific_unit_ {};
};

#endif // SPECIFICUNIT_H
