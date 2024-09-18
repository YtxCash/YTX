#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QFileOpenEvent>

class Application : public QApplication {
    Q_OBJECT

signals:
    // Signal emitted when a file is opened
    void SOpenFile(const QString& file_path);

public:
    // Constructor for the custom application class
    Application(int& argc, char** argv)
        : QApplication(argc, argv)
    {
    }

protected:
    // Override event handler to handle file open events
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::FileOpen) {
            // Cast the event to QFileOpenEvent
            auto open_event { static_cast<QFileOpenEvent*>(event) };
            // Emit the signal with the file path from the event
            emit SOpenFile(open_event->file());
        }

        // Pass the event to the base class event handler
        return QApplication::event(event);
    }
};

#endif // APPLICATION_H
