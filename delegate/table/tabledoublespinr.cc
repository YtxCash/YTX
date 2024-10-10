#include "tabledoublespinr.h"

TableDoubleSpinR::TableDoubleSpinR(const int& decimal, bool show_zero, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , show_zero_ { show_zero }
{
}

void TableDoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    if (value == 0 && !show_zero_)
        return StyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TableDoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option);
}
