#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QDate>
#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpressionValidator>

#include "component/constvalue.h"

class LineEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = nullptr)
        : QLineEdit(parent)
    {
    }

    static inline const QRegularExpression kInputRegex { QStringLiteral("[\\p{L} ()（）\\d]*") };
    static inline const QRegularExpressionValidator kInputValidator { kInputRegex };

protected:
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Semicolon) {
            insert(QDate::currentDate().toString(DATE_FST));
            return;
        }
        QLineEdit::keyPressEvent(event);
    }
};

#endif // LINEEDIT_H
