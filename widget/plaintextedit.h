#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QDate>
#include <QKeyEvent>
#include <QPlainTextEdit>

#include "component/constvalue.h"

class PlainTextEdit final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr)
        : QPlainTextEdit { parent }
    {
        setTabChangesFocus(true);
        setUndoRedoEnabled(true);
    }

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon)
            return insertPlainText(QDate::currentDate().toString(DATE_FST));

        QPlainTextEdit::keyPressEvent(event);
    }
};

#endif // PLAINTEXTEDIT_H
