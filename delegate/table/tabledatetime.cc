#include "tabledatetime.h"

#include "component/constvalue.h"
#include "widget/datetimeedit.h"

TableDateTime::TableDateTime(const QString& date_format, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
{
}

QWidget* TableDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    auto* editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(date_format_);

    return editor;
}

void TableDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto date_time { QDateTime::fromString(index.data().toString(), DATE_TIME_FST) };
    if (!date_time.isValid())
        date_time = last_date_time_.isValid() ? last_date_time_.addSecs(1) : QDateTime::currentDateTime();

    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    cast_ediotr->setDateTime(date_time);
}

void TableDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    auto date_time { cast_ediotr->dateTime() };

    last_date_time_ = date_time.date() == QDate::currentDate() ? QDateTime() : date_time;
    model->setData(index, date_time.toString(DATE_TIME_FST));
}

void TableDateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto date_time { index.data().toDateTime() };
    if (!date_time.isValid())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(date_time.toString(date_format_), painter, option, index, Qt::AlignCenter);
}

QSize TableDateTime::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto text { index.data().toDateTime().toString(date_format_) };
    return CalculateTextSize(text, option);
}
