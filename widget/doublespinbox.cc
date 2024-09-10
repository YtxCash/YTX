#include "doublespinbox.h"

#include <QKeyEvent>

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox { parent }
{
    this->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    this->setButtonSymbols(QAbstractSpinBox::NoButtons);
    this->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    this->setGroupSeparatorShown(true);
}

void DoubleSpinBox::wheelEvent(QWheelEvent* event) { event->ignore(); }

void DoubleSpinBox::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        if (text().isEmpty())
            setValue(0.0);
        Q_FALLTHROUGH();
    default:
        QDoubleSpinBox::keyPressEvent(event);
    }
}
