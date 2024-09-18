#ifndef TEMPORARYLABEL_H
#define TEMPORARYLABEL_H

#include <QLabel>
#include <QMouseEvent>

class TemporaryLabel : public QLabel {
    Q_OBJECT
public:
    explicit TemporaryLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(text, parent)
    {
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            this->close();
        }
        QLabel::mousePressEvent(event);
    }
};

#endif // TEMPORARYLABEL_H
