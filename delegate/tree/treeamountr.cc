#include "treeamountr.h"

#include <QPainter>

TreeAmountR::TreeAmountR(const int* decimal, CStringHash* unit_symbol_hash, const int* base_unit, QObject* parent)
    : QStyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_symbol_hash_ { unit_symbol_hash }
    , base_unit_ { base_unit }
    , locale_ { QLocale::English, QLocale::UnitedStates }
{
}

void TreeAmountR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto selected { option.state & QStyle::State_Selected };
    auto palette { option.palette };

    painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
    if (selected)
        painter->fillRect(option.rect, palette.highlight());

    painter->drawText(option.rect.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, FormattedString(index));
}

QSize TreeAmountR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(QFontMetrics(option.font).horizontalAdvance(FormattedString(index)) + 8, option.rect.height());
}

QString TreeAmountR::FormattedString(const QModelIndex& index) const
{
    return unit_symbol_hash_->value(*base_unit_) + locale_.toString(index.data().toDouble(), 'f', *decimal_);
}
