#include "checkbox.h"

#include <QMouseEvent>

CheckBox::CheckBox(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void CheckBox::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const { PaintCheckBox(painter, option, index); }

bool CheckBox::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);

        if (mouse_event->button() == Qt::LeftButton && option.rect.contains(mouse_event->pos())) {
            bool checked = index.data().toBool();
            return model->setData(index, !checked, Qt::EditRole);
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
