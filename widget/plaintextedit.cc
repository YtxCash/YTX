#include "plaintextedit.h"

#include <QDate>
#include <QKeyEvent>

#include "component/constvalue.h"

PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit { parent }
{
    this->setTabChangesFocus(true);
    this->setUndoRedoEnabled(true);
}

void PlainTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon)
        return this->insertPlainText(QDate::currentDate().toString(DATE_FST));

    QPlainTextEdit::keyPressEvent(event);
}
