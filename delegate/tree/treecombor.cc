#include "treecombor.h"

#include <widget/combobox.h>

TreeComboR::TreeComboR(CStringHash& hash, QObject* parent)
    : StyledItemDelegate { parent }
    , hash_ { hash }
{
}

void TreeComboR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(HashValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

QSize TreeComboR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = HashValue(index.data(Qt::EditRole).toInt());
    return CalculateTextSize(text, option);
}

QString TreeComboR::HashValue(int key) const
{
    static const QString empty_string {};
    auto it { hash_.constFind(key) };

    return (it != hash_.constEnd()) ? it.value() : empty_string;
}
