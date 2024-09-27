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
    EmptyText();
    QDoubleSpinBox::keyPressEvent(event);
}

void DoubleSpinBox::focusOutEvent(QFocusEvent* event)
{
    EmptyText();
    QDoubleSpinBox::focusOutEvent(event);
}
