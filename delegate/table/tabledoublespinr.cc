#include "tabledoublespinr.h"

#include <QPainter>

TableDoubleSpinR::TableDoubleSpinR(const int& decimal, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
{
}

void TableDoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    PaintItem(locale_.toString(value, 'f', decimal_), painter, option, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TableDoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    return CalculateSize(locale_.toString(value, 'f', decimal_), option, index);
}
