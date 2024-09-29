#ifndef SIGNALBLOCKER_H
#define SIGNALBLOCKER_H

#include <QWidget>

class SignalBlocker {
public:
    explicit SignalBlocker(QWidget* parent)
    {
        if (!parent)
            return;

        auto list { parent->findChildren<QWidget*>() };
        list.emplaceBack(parent);

        for (auto* widget : list)
            if (widget && !widget->signalsBlocked()) {
                widget->blockSignals(true);
                list_.emplace_back(widget);
            }
    }

    ~SignalBlocker()
    {
        for (auto* widget : list_)
            widget->blockSignals(false);
    }

    SignalBlocker(const SignalBlocker&) = delete;
    SignalBlocker& operator=(const SignalBlocker&) = delete;
    SignalBlocker(SignalBlocker&&) = delete;
    SignalBlocker& operator=(SignalBlocker&&) = delete;

private:
    std::vector<QWidget*> list_ {};
};

#endif // SIGNALBLOCKER_H
