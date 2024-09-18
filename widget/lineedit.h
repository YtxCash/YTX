#ifndef LINEEDIT_H
#define LINEEDIT_H

#include <QLineEdit>
#include <QRegularExpressionValidator>

class LineEdit final : public QLineEdit {
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = nullptr);
    static const QRegularExpressionValidator& GetInputValidator() { return kInputValidator; }

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    static const QRegularExpression kInputRegex;
    static const QRegularExpressionValidator kInputValidator;
};

#endif // LINEEDIT_H
