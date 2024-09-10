#ifndef PLAINTEXTEDIT_H
#define PLAINTEXTEDIT_H

#include <QPlainTextEdit>

class PlainTextEdit final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit PlainTextEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // PLAINTEXTEDIT_H
