#ifndef COLORR_H
#define COLORR_H

#include "delegate/styleditemdelegate.h"

class ColorR final : public StyledItemDelegate {
public:
    explicit ColorR(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // COLORR_H
