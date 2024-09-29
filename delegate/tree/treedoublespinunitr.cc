#include "treedoublespinunitr.h"

TreeDoubleSpinUnitR::TreeDoubleSpinUnitR(const int& decimal, const int& unit, CStringHash& unit_symbol_hash, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_ { unit }
    , unit_symbol_hash_ { unit_symbol_hash }
{
}

void TreeDoubleSpinUnitR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinUnitR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(Format(index), option); }

QString TreeDoubleSpinUnitR::Format(const QModelIndex& index) const
{
    static const QString empty_string {};

    auto it { unit_symbol_hash_.constFind(unit_) };
    auto symbol { (it != unit_symbol_hash_.constEnd()) ? it.value() : empty_string };

    return symbol + locale_.toString(index.data().toDouble(), 'f', decimal_);
}
