#include "orderlocked.h"

#include <QMouseEvent>

#include "component/enumclass.h"

OrderLocked::OrderLocked(QObject* parent)
    : StyledItemDelegate { parent }
{
}

void OrderLocked::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool() };
    if (branch)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintCheckBox(painter, option, index);
}

bool OrderLocked::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
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
