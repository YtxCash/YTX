#ifndef ORDERTOTAL_H
#define ORDERTOTAL_H

#include "delegate/styleditemdelegate.h"

class OrderTotalR final : public StyledItemDelegate {
public:
    OrderTotalR(const int& decimal, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int& decimal_;
};

#endif // ORDERTOTAL_H
