#include "treedoublespinunitr.h"

TreeDoubleSpinUnitR::TreeDoubleSpinUnitR(const int& decimal, bool ignore_zero, const int& unit, CStringMap& unit_symbol_map, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_ { unit }
    , unit_symbol_map_ { unit_symbol_map }
    , ignore_zero_ { ignore_zero }
{
}

void TreeDoubleSpinUnitR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    if (ignore_zero_ && value == 0.0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinUnitR::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const { return CalculateTextSize(Format(index)); }

QString TreeDoubleSpinUnitR::Format(const QModelIndex& index) const
{
    auto it { unit_symbol_map_.constFind(unit_) };
    auto symbol { (it != unit_symbol_map_.constEnd()) ? it.value() : EMPTYSTRING };

    return symbol + locale_.toString(index.data().toDouble(), 'f', decimal_);
}
