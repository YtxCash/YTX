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

#ifndef TREEDOUBLESPINUNITR_H
#define TREEDOUBLESPINUNITR_H

// read only

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class TreeDoubleSpinUnitR final : public StyledItemDelegate {
public:
    TreeDoubleSpinUnitR(const int& decimal, bool ignore_zero, const int& unit, CStringMap& unit_symbol_map, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QModelIndex& index) const;

private:
    const int& decimal_ {};
    const int& unit_ {};
    CStringMap& unit_symbol_map_;
    bool ignore_zero_ {};
};

#endif // TREEDOUBLESPINUNITR_H
