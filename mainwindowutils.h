#ifndef MAINWINDOWUTILS_H
#define MAINWINDOWUTILS_H

#include <QSettings>
#include <QWidget>

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tablewidget.h"

template <typename T>
concept InheritQAbstractItemView = std::is_base_of_v<QAbstractItemView, T>;

template <typename T>
concept InheritQWidget = std::is_base_of_v<QWidget, T>;

class MainWindowUtils {
public:
    static bool IsTreeWidget(const QWidget* widget) { return widget && widget->inherits("TreeWidget"); }
    static bool IsTableWidget(const QWidget* widget) { return widget && widget->inherits("TableWidget"); }

    static PTableModel GetTableModel(QWidget* widget)
    {
        if (!widget)
            return nullptr;

        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        return static_cast<TableWidget*>(widget)->Model();
    }

    static PQTableView GetQTableView(QWidget* widget)
    {
        if (!widget)
            return nullptr;

        assert(dynamic_cast<TableWidget*>(widget) && "Widget is not TableWidget");
        return static_cast<TableWidget*>(widget)->View();
    }

    template <InheritQAbstractItemView T> static bool HasSelection(QPointer<T> view)
    {
        return view && view->selectionModel() && view->selectionModel()->hasSelection();
    }

    template <InheritQWidget T> static void FreeWidget(T*& widget)
    {
        if (widget) {
            if (auto model = widget->Model())
                delete model;

            delete widget;
            widget = nullptr;
        }
    }

    template <typename Container> static void SwitchDialog(Container* container, bool enable)
    {
        if (container) {
            for (auto dialog : *container) {
                if (dialog) {
                    dialog->setVisible(enable);
                }
            }
        }
    }

    template <InheritQWidget T> static void SaveState(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto state { widget->saveState() };
        interface->setValue(QString("%1/%2").arg(section_name, property), state);
    }

    template <InheritQWidget T> static void RestoreState(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto state { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };

        if (!state.isEmpty())
            widget->restoreState(state);
    }

    template <InheritQWidget T> static void SaveGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto geometry { widget->saveGeometry() };
        interface->setValue(QString("%1/%2").arg(section_name, property), geometry);
    }

    template <InheritQWidget T> static void RestoreGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        auto geometry { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };
        if (!geometry.isEmpty())
            widget->restoreGeometry(geometry);
    }
};

#endif // MAINWINDOWUTILS_H