/*
 * Copyright (C) 2023 YtxCash
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

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
    static bool IsEditNodeOrder(const QWidget* widget) { return widget && widget->inherits("EditNodeOrder"); }

    static QVariantList SaveTab(CTableHash& table_hash);
    static void WriteTabID(QSettings* interface, const QVariantList& list, CString& section_name, CString& property);
    static QSet<int> ReadTabID(QSettings* interface, CString& section_name, CString& property);

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
        if (!widget || !interface) {
            qWarning() << "SaveState: Invalid parameters (widget or interface is null)";
            return;
        }

        auto state { widget->saveState() };
        interface->setValue(QString("%1/%2").arg(section_name, property), state);
    }

    template <InheritQWidget T> static void RestoreState(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        if (!widget || !interface) {
            qWarning() << "RestoreState: Invalid parameters (widget or interface is null)";
            return;
        }

        auto state { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };

        if (!state.isEmpty())
            widget->restoreState(state);
    }

    template <InheritQWidget T> static void SaveGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        if (!widget || !interface) {
            qWarning() << "SaveGeometry: Invalid parameters (widget or interface is null)";
            return;
        }

        auto geometry { widget->saveGeometry() };
        interface->setValue(QString("%1/%2").arg(section_name, property), geometry);
    }

    template <InheritQWidget T> static void RestoreGeometry(T* widget, QSettings* interface, CString& section_name, CString& property)
    {
        if (!widget || !interface) {
            qWarning() << "RestoreGeometry: Invalid parameters (widget or interface is null)";
            return;
        }

        auto geometry { interface->value(QString("%1/%2").arg(section_name, property)).toByteArray() };
        if (!geometry.isEmpty())
            widget->restoreGeometry(geometry);
    }
};

#endif // MAINWINDOWUTILS_H
