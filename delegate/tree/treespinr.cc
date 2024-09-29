#include "treespinr.h"

TreeSpinR::TreeSpinR(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void TreeSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    return CalculateTextSize(locale_.toString(value), option);
}
