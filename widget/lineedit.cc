#include "lineedit.h"

#include <QDate>
#include <QKeyEvent>

#include "component/constvalue.h"

const QRegularExpression LineEdit::kInputRegex(QStringLiteral("[\\p{L} ()（）\\d]*"));
const QRegularExpressionValidator LineEdit::kInputValidator(LineEdit::kInputRegex);

LineEdit::LineEdit(QWidget* parent)
    : QLineEdit { parent }
{
}

void LineEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon)
        return insert(QDate::currentDate().toString(DATE_FST));

    QLineEdit::keyPressEvent(event);
}
