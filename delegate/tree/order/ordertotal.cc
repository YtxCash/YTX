#include "ordertotal.h"

#include <QPainter>

OrderTotalR::OrderTotalR(const int& decimal, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
{
}

void OrderTotalR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize OrderTotalR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option);
}
