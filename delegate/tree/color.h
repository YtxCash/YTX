#ifndef COLOR_H
#define COLOR_H

#include "delegate/styleditemdelegate.h"

class Color final : public StyledItemDelegate {
public:
    explicit Color(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
};

#endif // COLOR_H
