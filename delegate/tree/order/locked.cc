#include "locked.h"

#include <QMouseEvent>

#include "component/enumclass.h"

Locked::Locked(QEvent::Type type, QObject* parent)
    : StyledItemDelegate { parent }
    , type_ { type }
{
}

void Locked::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.data().toBool())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintCheckBox(painter, option, index);
}

bool Locked::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() != type_ || index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool())
        return false;

    auto* mouse_event { static_cast<QMouseEvent*>(event) };
    if (mouse_event->button() != Qt::LeftButton || !option.rect.contains(mouse_event->pos()))
        return false;

    const bool checked { index.data().toBool() };
    return model->setData(index, !checked, Qt::EditRole);
}
