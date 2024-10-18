#include "orderdatetime.h"

#include <QPainter>

#include "component/enumclass.h"
#include "widget/datetimeedit.h"

OrderDateTime::OrderDateTime(const QString& date_format, bool skip_branch, QObject* parent)
    : StyledItemDelegate { parent }
    , skip_branch_ { skip_branch }
    , date_format_ { date_format }
{
}

QWidget* OrderDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    if (skip_branch_ && index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool())
        return nullptr;

    auto editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(date_format_);

    return editor;
}

void OrderDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DateTimeEdit*>(editor) };
    auto date_time { QDateTime::fromString(index.data().toString(), date_format_) };

    cast_editor->setDateTime(date_time);
}

void OrderDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DateTimeEdit*>(editor) };
    auto string { cast_editor->dateTime().toString(date_format_) };
    model->setData(index, string);
}

void OrderDateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto string { index.data().toString() };
    if (string.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(string, painter, option, index, Qt::AlignCenter);
}
