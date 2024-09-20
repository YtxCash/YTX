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
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (text().isEmpty())
            setValue(0);
        Q_FALLTHROUGH();
    default:
        QSpinBox::keyPressEvent(event);
    }
}
