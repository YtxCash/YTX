#ifndef TREECOMBO_H
#define TREECOMBO_H

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class TreeCombo final : public StyledItemDelegate {
public:
    TreeCombo(CStringHash& hash, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString HashValue(int key) const;

private:
    CStringHash& hash_;
};

#endif // TREECOMBO_H
