#ifndef ORDERPOSTED_H
#define ORDERPOSTED_H

// tree's branch column 6, table's state column 7, arte different, so they can share this delegate

#include "delegate/styleditemdelegate.h"

class OrderPosted final : public StyledItemDelegate {
public:
    OrderPosted(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    mutable QRect rect_ {};
};

#endif // ORDERPOSTED_H
