#include "treedoublespinunitr.h"

#include <QPainter>

#include "component/enumclass.h"

TreeDoubleSpinUnitR::TreeDoubleSpinUnitR(const int& decimal, CStringHash& unit_symbol_hash, bool format_zero, QObject* parent)
    : QStyledItemDelegate { parent }
    , decimal_ { decimal }
    , format_zero_ { format_zero }
    , unit_symbol_hash_ { unit_symbol_hash }
    , locale_ { QLocale::English, QLocale::UnitedStates }
{
}

void TreeDoubleSpinUnitR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto selected { option.state & QStyle::State_Selected };
    auto palette { option.palette };

    painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
    if (selected)
        painter->fillRect(option.rect, palette.highlight());

    painter->drawText(option.rect.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, Format(index));
}

QSize TreeDoubleSpinUnitR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(QFontMetrics(option.font).horizontalAdvance(Format(index)) + 8, option.rect.height());
}

QString TreeDoubleSpinUnitR::Format(const QModelIndex& index) const
{
    if (!format_zero_ && index.data().toDouble() == 0)
        return QString();

    int unit { index.sibling(index.row(), std::to_underlying(TreeEnum::kUnit)).data().toInt() };
    return unit_symbol_hash_.value(unit, QString()) + locale_.toString(index.data().toDouble(), 'f', decimal_);
}
