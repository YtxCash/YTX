#include "checkbox.h"

#include <QMouseEvent>

CheckBox::CheckBox(QEvent::Type type, QObject* parent)
    : StyledItemDelegate { parent }
    , type_ { type }
{
}

void CheckBox::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const { PaintCheckBox(painter, option, index); }

bool CheckBox::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() != type_)
        return false;

    auto mouse_event { static_cast<QMouseEvent*>(event) };
    if (mouse_event->button() != Qt::LeftButton || !option.rect.contains(mouse_event->pos()))
        return false;

    const bool checked { index.data().toBool() };
    return model->setData(index, !checked, Qt::EditRole);
}
