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

#ifndef TABLEVIEWORDER_H
#define TABLEVIEWORDER_H

#include <QTableView>

// enter 添加新行不实用，还是主动控制添加好

class TableViewOrder : public QTableView {
    Q_OBJECT
public:
    explicit TableViewOrder(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // TABLEVIEWORDER_H
