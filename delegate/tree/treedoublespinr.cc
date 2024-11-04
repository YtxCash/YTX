#include "treedoublespinr.h"

TreeDoubleSpinR::TreeDoubleSpinR(const int& decimal, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
{
}

void TreeDoubleSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_), option);
}
