#ifndef SPINBOX_H
#define SPINBOX_H

#include <QSpinBox>
#include <QWheelEvent>

class SpinBox final : public QSpinBox {
    Q_OBJECT

public:
    explicit SpinBox(QWidget* parent = nullptr)
        : QSpinBox { parent }
    {
        setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
        setGroupSeparatorShown(true);
    }

protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
    void keyPressEvent(QKeyEvent* event) override
    {
        if (cleanText().isEmpty())
            setValue(0);

        QSpinBox::keyPressEvent(event);
    }
};

#endif // SPINBOX_H
