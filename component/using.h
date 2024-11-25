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

#ifndef USING_H
#define USING_H

#include <QHash>
#include <QMap>
#include <QStringList>

using CStringMap = const QMap<int, QString>;
using StringMap = QMap<int, QString>;

using CStringHash = const QHash<int, QString>;
using StringHash = QHash<int, QString>;

using CString = const QString;
using CVariant = const QVariant;
using CStringList = const QStringList;

inline const QString kEmptyString = {};

#endif // USING_H
