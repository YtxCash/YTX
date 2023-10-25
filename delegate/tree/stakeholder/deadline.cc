#include "deadline.h"

#include <QPainter>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/datetimeedit.h"

Deadline::Deadline(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

QWidget* Deadline::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    return new DateTimeEdit(DATE_D, parent);
}

void Deadline::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto date_time { index.sibling(index.row(), std::to_underlying(TreeEnumStakeholder::kNodeRule)).data().toBool() == 0
            ? QDateTime()
            : QDateTime::fromString(index.data().toString(), DATE_D) };

    qobject_cast<DateTimeEdit*>(editor)->setDateTime(date_time);
}

void Deadline::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

void Deadline::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto string { index.sibling(index.row(), std::to_underlying(TreeEnumStakeholder::kNodeRule)).data().toBool() == 0
            ? QString()
            : qobject_cast<DateTimeEdit*>(editor)->dateTime().toString(DATE_D) };

    model->setData(index, string);
}

void Deadline::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto string { index.data().toString() };
    if (string.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    auto selected { option.state & QStyle::State_Selected };
    auto palette { option.palette };

    painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
    if (selected)
        painter->fillRect(option.rect, palette.highlight());

    painter->drawText(option.rect.adjusted(4, 0, 0, 0), Qt::AlignCenter, string);
}
