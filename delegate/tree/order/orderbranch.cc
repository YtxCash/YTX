#include "orderbranch.h"

#include <QMouseEvent>

OrderBranch::OrderBranch(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void OrderBranch::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if(!index.data().toBool())
        return QStyledItemDelegate::paint(painter, option, index);

     PaintCheckBox(painter, option, index); }

bool OrderBranch::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
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
