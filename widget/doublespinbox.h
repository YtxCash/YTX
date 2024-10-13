#ifndef DOUBLESPINBOX_H
#define DOUBLESPINBOX_H

#include <QDoubleSpinBox>
#include <QKeyEvent>
#include <QLineEdit>

#include "component/constvalue.h"

class DoubleSpinBox final : public QDoubleSpinBox {
    Q_OBJECT

public:
    explicit DoubleSpinBox(QWidget* parent = nullptr)
        : QDoubleSpinBox { parent }
    {
        setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
        setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        setGroupSeparatorShown(true);
    }

protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); };
    void keyPressEvent(QKeyEvent* event) override
    {
        if (event->text() == QString::fromUtf8(FULL_WIDTH_PERIOD)) {
            QKeyEvent new_event(QEvent::KeyPress, Qt::Key_Period, Qt::NoModifier, HALF_WIDTH_PERIOD);
            QDoubleSpinBox::keyPressEvent(&new_event);
            return;
        }

        if (cleanText().isEmpty()) {
            setValue(DZERO);
        }

        QDoubleSpinBox::keyPressEvent(event);
    }
};

#endif // DOUBLESPINBOX_H
