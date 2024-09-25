#include "spinbox.h"

#include <QWheelEvent>

SpinBox::SpinBox(QWidget* parent)
    : QSpinBox { parent }
{
    this->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    this->setGroupSeparatorShown(true);
}

void SpinBox::wheelEvent(QWheelEvent* event) { event->ignore(); }

void SpinBox::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (cleanText().isEmpty()) {
            setValue(0);
            return;
        }
    default:
        QSpinBox::keyPressEvent(event);
    }
}
