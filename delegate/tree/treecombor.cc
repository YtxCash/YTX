#include "treecombor.h"

#include <widget/combobox.h>

TreeComboR::TreeComboR(CStringMap& map, QObject* parent)
    : StyledItemDelegate { parent }
    , map_ { map }
{
}

void TreeComboR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(MapValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

QSize TreeComboR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = MapValue(index.data().toInt());
    return CalculateTextSize(text, option);
}

QString TreeComboR::MapValue(int key) const
{
    static const QString empty_string {};
    auto it { map_.constFind(key) };

    return (it != map_.constEnd()) ? it.value() : empty_string;
}
