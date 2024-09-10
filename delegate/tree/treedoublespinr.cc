#include "treedoublespinr.h"

#include <QPainter>

TreeDoubleSpinR::TreeDoubleSpinR(const int& decimal, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
{
}

void TreeDoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(locale_.toString(value, 'f', decimal_), painter, option, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    return CalculateSize(locale_.toString(value, 'f', decimal_), option, index);
}
