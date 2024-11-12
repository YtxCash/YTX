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

#ifndef DPIHELPER_H
#define DPIHELPER_H

#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QWidget>
#include <QWindow>

class DPIHelper {
public:
    static constexpr qreal STANDARD_DPI = 96.0;

    static void SetApplicationDPI()
    {
        // 获取主屏幕
        QScreen* screen { QGuiApplication::primaryScreen() };
        if (!screen)
            return;

        // 获取屏幕逻辑DPI
        qreal logical_dpi { screen->logicalDotsPerInch() };

        // 计算缩放因子
        qreal scale_factor { logical_dpi / STANDARD_DPI };

        // 设置应用程序级别的缩放因子
        qputenv("QT_SCALE_FACTOR", QString::number(scale_factor).toLatin1());
    }

    static void AdjustWidgetDPI(QWidget* widget)
    {
        if (!widget)
            return;

        // 获取窗口所在屏幕
        QScreen* screen { widget->window()->screen() };
        if (!screen)
            return;

        // 获取当前屏幕DPI
        qreal dpi { screen->logicalDotsPerInch() };

        // 调整字体大小
        QFont font { widget->font() };

        int font_size { static_cast<int>(font.pointSize() * (dpi / STANDARD_DPI)) };
        font.setPointSize(font_size);
        widget->setFont(font);

        // 递归处理所有子控件
        for (QObject* child : widget->children()) {
            if (QWidget* child_widget = qobject_cast<QWidget*>(child)) {
                AdjustWidgetDPI(child_widget);
            }
        }
    }
};

#endif // DPIHELPER_H
