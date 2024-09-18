#include "datetime.h"

#include "component/constvalue.h"
#include "widget/datetimeedit.h"

DateTime::DateTime(const QString& date_format, const bool& hide_time, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
    , hide_time_ { hide_time }
    , time_pattern_ { R"((\s?\S{2}:\S{2}\s?))" }
{
}

QWidget* DateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    auto format { date_format_ };
    if (hide_time_)
        format.remove(time_pattern_);

    auto editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(format);

    return editor;
}

void DateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto date_time { index.data().toDateTime() };
    if (!date_time.isValid())
        date_time = last_date_time_.isValid() ? last_date_time_.addSecs(1) : QDateTime::currentDateTime();

    qobject_cast<DateTimeEdit*>(editor)->setDateTime(date_time);
}

void DateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto date_time { qobject_cast<DateTimeEdit*>(editor)->dateTime() };
    last_date_time_ = date_time.date() == QDate::currentDate() ? QDateTime() : date_time;

    model->setData(index, date_time.toString(DATE_TIME_FST));
}

void DateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto date_time { index.data().toDateTime() };
    if (!date_time.isValid())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(Format(date_time), painter, option, index, Qt::AlignCenter);
}

QSize DateTime::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto text { Format(index.data().toDateTime()) };
    return CalculateTextSize(text, option);
}

QString DateTime::Format(const QDateTime& date_time) const
{
    if (!date_time.isValid())
        return QString();

    auto format { date_format_ };
    if (hide_time_)
        format.remove(time_pattern_);

    return date_time.toString(format);
}
