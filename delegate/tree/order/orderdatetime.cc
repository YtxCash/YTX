#include "orderdatetime.h"

#include <QPainter>

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/datetimeedit.h"

OrderDateTime::OrderDateTime(const QString& date_format, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
{
}

QWidget* OrderDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool() };
    if (branch)
        return nullptr;

    auto editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(date_format_);

    return editor;
}

void OrderDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    qobject_cast<DateTimeEdit*>(editor)->setDateTime(index.data().toDateTime());
}

void OrderDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto string { qobject_cast<DateTimeEdit*>(editor)->dateTime().toString(DATE_TIME_FST) };
    model->setData(index, string);
}

void OrderDateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto string { index.data().toString() };
    if (string.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(string, painter, option, Qt::AlignCenter);
}
