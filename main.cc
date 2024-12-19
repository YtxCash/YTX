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

#include <QDir>

#include "mainwindow.h"

#ifdef Q_OS_MACOS
#include "component/application.h"

int main(int argc, char* argv[])
{
    Application application(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // Centralize config directory creation
    const QString config_dir { QDir::home().filePath(".config/YTX") };

    if (!QDir(config_dir).exists() && !QDir::home().mkpath(config_dir)) {
        qCritical() << "Failed to create config directory:" << config_dir;
        return EXIT_FAILURE;
    }

    MainWindow mainwindow(config_dir);

    if (application.arguments().size() >= 2) {
        const QString file_path { application.arguments().at(1) };

        if (!file_path.isEmpty() && !mainwindow.ROpenFile(file_path)) {
            return EXIT_FAILURE;
        }
    }

    QObject::connect(&application, &Application::SOpenFile, &mainwindow, &MainWindow::ROpenFile);

    mainwindow.show();
    return application.exec();
}
#endif

#ifdef Q_OS_WIN
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // Centralize config directory creation
    const QString config_dir { QDir::home().filePath("/AppData/Local/YTX") };

    if (!QDir(config_dir).exists() && !QDir::home().mkpath(config_dir)) {
        qCritical() << "Failed to create config directory:" << config_dir;
        return EXIT_FAILURE;
    }

    MainWindow mainwindow(config_dir);

    // Simplified file handling and locking
    if (argc >= 2) {
        const QString file_path { QString::fromLocal8Bit(argv[1]) };

        if (!file_path.isEmpty() && !mainwindow.ROpenFile(file_path)) {
            return EXIT_FAILURE;
        }
    }

    mainwindow.show();
    return application.exec();
}
#endif
