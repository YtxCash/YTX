#ifndef DOUBLESPINBOX_H
#define DOUBLESPINBOX_H

#include <QDoubleSpinBox>

class DoubleSpinBox final : public QDoubleSpinBox {
    Q_OBJECT

public:
    DoubleSpinBox(QWidget* parent = nullptr);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    inline void EmptyText()
    {
        if (cleanText().isEmpty())
            setValue(0.0);
    }
};

#endif // DOUBLESPINBOX_H
