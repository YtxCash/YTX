#include "treedoublespindynamicunitr.h"

#include "component/enumclass.h"

TreeDoubleSpinDynamicUnitR::TreeDoubleSpinDynamicUnitR(const int& decimal, CStringHash& unit_symbol_hash, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , unit_symbol_hash_ { unit_symbol_hash }
{
}

void TreeDoubleSpinDynamicUnitR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinDynamicUnitR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return CalculateTextSize(Format(index), option);
}

QString TreeDoubleSpinDynamicUnitR::Format(const QModelIndex& index) const
{
    static const QString empty_string {};
    double value { index.data().toDouble() };

    if (value == 0)
        return empty_string;

    int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };
    auto it { unit_symbol_hash_.constFind(unit) };
    auto symbol { (it != unit_symbol_hash_.constEnd()) ? it.value() : empty_string };

    return symbol + locale_.toString(value, 'f', decimal_);
}
