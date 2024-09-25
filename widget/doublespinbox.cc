#include "doublespinbox.h"

#include <QKeyEvent>

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox { parent }
{
    this->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    this->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    this->setGroupSeparatorShown(true);
}

void DoubleSpinBox::wheelEvent(QWheelEvent* event) { event->ignore(); }

void DoubleSpinBox::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (cleanText().isEmpty()) {
            setValue(0.0);
            return;
        }
    default:
        QDoubleSpinBox::keyPressEvent(event);
    }
}
