#include "checkboxr.h"

#include <QMouseEvent>

CheckBoxR::CheckBoxR( QObject* parent)
    : StyledItemDelegate { parent }
{
}

void CheckBoxR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const { PaintCheckBox(painter, option, index); }
