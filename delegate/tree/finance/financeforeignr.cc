#include "financeforeignr.h"

#include "component/enumclass.h"

FinanceForeignR::FinanceForeignR(const int& decimal, const int& default_unit, CStringHash& unit_symbol_hash, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , default_unit_ { default_unit }
    , unit_symbol_hash_ { unit_symbol_hash }
{
}

void FinanceForeignR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(Format(index), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize FinanceForeignR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(Format(index), option); }

QString FinanceForeignR::Format(const QModelIndex& index) const
{
    static const QString empty_string {};

    int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };
    if (unit == default_unit_)
        return empty_string;

    double value { index.data().toDouble() };

    auto it { unit_symbol_hash_.constFind(unit) };
    auto symbol { (it != unit_symbol_hash_.constEnd()) ? it.value() : empty_string };

    return symbol + locale_.toString(value, 'f', decimal_);
}
