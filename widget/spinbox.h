#ifndef SPINBOX_H
#define SPINBOX_H

#include <QSpinBox>

class SpinBox final : public QSpinBox {
    Q_OBJECT

public:
    SpinBox(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    inline void EmptyText()
    {
        if (cleanText().isEmpty())
            setValue(0);
    }
};

#endif // SPINBOX_H
