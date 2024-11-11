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

#ifndef STYLEDITEMDELEGATE_H
#define STYLEDITEMDELEGATE_H

#include <QLocale>
#include <QStyledItemDelegate>

#include "component/using.h"

class StyledItemDelegate : public QStyledItemDelegate {
public:
    explicit StyledItemDelegate(QObject* parent = nullptr);
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    static void SetFontMetrics();
    static void SetTextMargin();
    static QSize CalculateTextSize(CString& text);

protected:
    void PaintText(CString& text, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, Qt::Alignment alignment) const;
    void PaintCheckBox(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

protected:
    static const QLocale locale_;
    static std::optional<QFontMetrics> fm_;
    static std::optional<int> text_margin_;
};

#endif // STYLEDITEMDELEGATE_H
