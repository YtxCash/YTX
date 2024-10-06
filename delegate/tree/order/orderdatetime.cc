#include "orderdatetime.h"

#include <QPainter>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/datetimeedit.h"

OrderDateTime::OrderDateTime(QObject* parent)
    : StyledItemDelegate { parent }
{
}

QWidget* OrderDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool() };
    if (branch)
        return nullptr;

    auto editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(DATE_TIME_FST);

    return editor;
}

void OrderDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DateTimeEdit*>(editor) };
    cast_editor->setDateTime(index.data().toDateTime());
}

void OrderDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DateTimeEdit*>(editor) };
    auto string { cast_editor->dateTime().toString(DATE_TIME_FST) };
    model->setData(index, string);
}

void OrderDateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto string { index.data().toString() };
    if (string.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(string, painter, option, index, Qt::AlignCenter);
}
