#include "datedelegate.h"
#include <QDate>
#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

DateDelegate::DateDelegate(const QString& format, QObject* parent)
    : QStyledItemDelegate { parent }
    , format_ { format }
{
    separator_ = GetSeparator(format);
}

QWidget* DateDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto line_editor = new QLineEdit(parent);

    QString re_pattern = QString("^(\\d{4})%1(0[1-9]|1[0-2])%1(0[1-9]|[1-2][0-9]|3[0-1])$").arg(separator_);
    QRegularExpression re(re_pattern);

    auto validator = new QRegularExpressionValidator(re, line_editor);

    line_editor->setValidator(validator);

    return line_editor;
}

void DateDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);

    auto date = index.data().toDate();
    auto text = date.toString(format_);
    line_editor->setText(text);
}

void DateDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}

void DateDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);
    auto text = line_editor->text();

    auto date = QDate::fromString(text, format_);
    if (date.isValid())
        model->setData(index, date);
}

QString DateDelegate::GetSeparator(const QString& format)
{
    static QRegularExpression separator_regex("(\\W)");
    auto match = separator_regex.match(format);

    if (match.hasMatch()) {
        return match.captured(1);
    } else {
        return QString();
    }
}

void DateDelegate::LastMonthEnd(QDate& date)
{
    auto last_month_date = date.addMonths(-1);
    auto last_month_end = QDate(last_month_date.year(), last_month_date.month(), last_month_date.daysInMonth());
    date = last_month_end;
}

void DateDelegate::NextMonthStart(QDate& date)
{
    auto next_month_date = date.addMonths(1);
    auto next_month_start = QDate(next_month_date.year(), next_month_date.month(), 1);
    date = next_month_start;
}

bool DateDelegate::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress)
        return QStyledItemDelegate::eventFilter(watched, event);

    auto line_editor = qobject_cast<QLineEdit*>(watched);
    auto date = QDate::fromString(line_editor->text(), format_);
    auto key = static_cast<QKeyEvent*>(event)->key();

    switch (key) {
    case Qt::Key_H:
        date = date.addDays(-1);
        break;
    case Qt::Key_L:
        date = date.addDays(1);
        break;
    case Qt::Key_J:
        date = date.addDays(7);
        break;
    case Qt::Key_K:
        date = date.addDays(-7);
        break;
    case Qt::Key_W:
        NextMonthStart(date);
        break;
    case Qt::Key_B:
        LastMonthEnd(date);
        break;
    case Qt::Key_N:
        date = date.addYears(-1);
        break;
    case Qt::Key_E:
        date = date.addYears(1);
        break;
    case Qt::Key_T:
        date = QDate::currentDate();
        break;
    default:
        return QStyledItemDelegate::eventFilter(watched, event);
    }

    if (date.isValid()) {
        line_editor->setText(date.toString(format_));
        return true;
    }

    return QStyledItemDelegate::eventFilter(watched, event);
}
