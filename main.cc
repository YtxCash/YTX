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

#include <QApplication>
#include <QDir>
#include <QLockFile>

#include "component/constvalue.h"
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

    // Simplified file handling and locking
    if (application.arguments().size() >= 2) {
        const QString file_path { application.arguments().at(1) };

        const QFileInfo file_info(file_path);
        if (!file_info.exists() || file_info.suffix().toLower() != "ytx") {
            qCritical() << "Invalid file path: must be an existing .ytx file";
            return EXIT_FAILURE;
        }

        const QFileInfo file_info(file_path);
        const QString lock_file_path { file_info.dir().filePath(file_info.completeBaseName() + kSuffixLOCK) };

        static QLockFile lock_file { lock_file_path };
        if (!lock_file.tryLock(100)) {
            qCritical() << "Unable to lock database file. Ensure no other instance of the application is using the file:" << lock_file_path;
            return EXIT_FAILURE;
        }

        mainwindow.ROpenFile(file_path);
    }

    QObject::connect(&application, &Application::SOpenFile, &mainwindow, &MainWindow::ROpenFile);
    mainwindow.show();
    return application.exec();
}
#endif

#ifdef Q_OS_WIN
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

        const QFileInfo file_info(file_path);
        if (!file_info.exists() || file_info.suffix().toLower() != "ytx") {
            qCritical() << "Invalid file path: must be an existing .ytx file";
            return EXIT_FAILURE;
        }

        const QFileInfo file_info(file_path);
        const QString lock_file_path { file_info.dir().filePath(file_info.completeBaseName() + kSuffixLOCK) };

        static QLockFile lock_file { lock_file_path };
        if (!lock_file.tryLock(100)) {
            qCritical() << "Unable to lock database file. Ensure no other instance of the application is using the file:" << lock_file_path;
            return EXIT_FAILURE;
        }

        mainwindow.ROpenFile(file_path);
    }

    mainwindow.show();
    return application.exec();
}
#endif
