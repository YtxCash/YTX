#include "alignright.h"

AlignRight::AlignRight(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

void AlignRight::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    QStyledItemDelegate::paint(painter, opt, index);
}
