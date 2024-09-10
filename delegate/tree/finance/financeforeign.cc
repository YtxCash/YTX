#include "financeforeign.h"

#include <QPainter>

#include "component/enumclass.h"

FinanceForeign::FinanceForeign(const int& decimal, const int& base_unit, CStringHash& unit_symbol_hash, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , base_unit_ { base_unit }
    , unit_symbol_hash_ { unit_symbol_hash }
{
}

void FinanceForeign::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintItem(Format(index), painter, option, Qt::AlignRight | Qt::AlignVCenter);
}

QSize FinanceForeign::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateSize(Format(index), option, index); }

QString FinanceForeign::Format(const QModelIndex& index) const
{
    static const QString empty_string {};

    int unit { index.siblingAtColumn(std::to_underlying(TreeEnum::kUnit)).data().toInt() };
    if (unit == base_unit_)
        return empty_string;

    double value { index.data().toDouble() };

    auto it { unit_symbol_hash_.constFind(unit) };
    auto symbol { (it != unit_symbol_hash_.constEnd()) ? *it : empty_string };

    return symbol + locale_.toString(value, 'f', decimal_);
}
