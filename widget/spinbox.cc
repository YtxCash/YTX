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
    EmptyText();
    QSpinBox::keyPressEvent(event);
}

void SpinBox::focusOutEvent(QFocusEvent* event)
{
    EmptyText();
    QSpinBox::focusOutEvent(event);
}
