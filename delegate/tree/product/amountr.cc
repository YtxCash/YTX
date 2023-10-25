#include "amountr.h"

#include <QPainter>

AmountR::AmountR(const int& decimal, CStringHash& currency_symbol_hash, const int& base_currency, QObject* parent)
    : QStyledItemDelegate { parent }
    , decimal_ { decimal }
    , currency_symbol_hash_ { currency_symbol_hash }
    , base_currency_ { base_currency }
    , locale_ { QLocale::English, QLocale::UnitedStates }
{
}

void AmountR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto selected { option.state & QStyle::State_Selected };
    auto palette { option.palette };

    painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
    if (selected)
        painter->fillRect(option.rect, palette.highlight());

    painter->drawText(option.rect.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, FormattedString(index));
}

QSize AmountR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(QFontMetrics(option.font).horizontalAdvance(FormattedString(index)) + 8, option.rect.height());
}

QString AmountR::FormattedString(const QModelIndex& index) const
{
    return currency_symbol_hash_.value(base_currency_) + locale_.toString(index.data().toDouble(), 'f', decimal_);
}
