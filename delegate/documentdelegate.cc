#include "documentdelegate.h"
#include <QKeyEvent>
#include <QPainter>

DocumentDelegate::DocumentDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

void DocumentDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto opt = option;
    opt.displayAlignment = Qt::AlignCenter;
    QStyledItemDelegate::paint(painter, opt, index);
}

bool DocumentDelegate::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto key_event = static_cast<QKeyEvent*>(event);
        auto key = key_event->key();

        if (key == Qt::Key_Tab || key == Qt::Key_Return || key == Qt::Key_Enter)
            return QStyledItemDelegate::eventFilter(watched, event);

        if (key_event->modifiers() == Qt::ShiftModifier)
            return QStyledItemDelegate::eventFilter(watched, event);
    }

    return true;
}
