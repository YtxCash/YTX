#ifndef TABLECOMBO_H
#define TABLECOMBO_H

#include <QStandardItemModel>

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class TableCombo final : public StyledItemDelegate {
public:
    TableCombo(CTreeModel* tree_model, int exclude_id, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

public slots:
    void RUpdateComboModel();

private:
    int exclude_id_ {};
    mutable int last_insert_ {};
    CTreeModel* tree_model_ {};

    QStandardItemModel* combo_model_ {};
};

#endif // TABLECOMBO_H
