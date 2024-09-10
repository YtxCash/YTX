#include "treespinr.h"

#include <QPainter>

TreeSpinR::TreeSpinR(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void TreeSpinR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toInt() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(locale_.toString(value), painter, option, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeSpinR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toInt() };
    return CalculateSize(locale_.toString(value), option, index);
}
