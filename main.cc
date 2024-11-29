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
#endif

int main(int argc, char* argv[])
{
// begin set ini file directory
#ifdef Q_OS_WIN
    QApplication application(argc, argv);
    QString dir_path { QDir::homePath() + "/AppData/Roaming/" + kYTX };
#elif defined(Q_OS_MACOS)
    Application application(argc, argv);
    QString dir_path { QDir::homePath() + "/.config/" + kYTX };
#endif

    if (QDir dir { dir_path }; !dir.exists()) {
        if (!QDir::home().mkpath(dir_path)) {
            qDebug() << "Failed to create directory:" << dir_path;
            return 1;
        }
    }
    // end set ini file directory

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    // begin set database file, then try to lock it, if false return
    QString file_path {};

    if (application.arguments().size() >= 2) {
#ifdef Q_OS_WIN
        file_path = QString::fromLocal8Bit(argv[1]);
#elif defined(Q_OS_MACOS)
        file_path = application.arguments().at(1);
#endif
    }

    if (!file_path.isEmpty()) {
        QFileInfo file_info(file_path);
        auto lock_file_path { file_info.absolutePath() + kSlash + file_info.completeBaseName() + kSuffixLOCK };

        static QLockFile lock_file { lock_file_path };
        if (!lock_file.tryLock(100))
            return 1;
    }
    // end set database file

    MainWindow w(dir_path);
    w.show();

    if (!file_path.isEmpty())
        w.ROpenFile(file_path);

#ifdef Q_OS_MACOS
    QObject::connect(&application, &Application::SOpenFile, &w, &MainWindow::ROpenFile);
#endif

    return application.exec();
}
