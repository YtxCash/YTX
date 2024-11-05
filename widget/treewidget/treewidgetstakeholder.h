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

#ifndef TREEWIDGETSTAKEHOLDER_H
#define TREEWIDGETSTAKEHOLDER_H

#include "component/info.h"
#include "component/settings.h"
#include "treewidget.h"

namespace Ui {
class TreeWidgetStakeholder;
}

class TreeWidgetStakeholder final : public TreeWidget {
    Q_OBJECT

public:
    TreeWidgetStakeholder(TreeModel* model, CInfo& info, CSettings& settings, QWidget* parent = nullptr);
    ~TreeWidgetStakeholder() override;

    QPointer<QTreeView> View() const override;
    QPointer<TreeModel> Model() const override { return model_; };

private:
    Ui::TreeWidgetStakeholder* ui;

    TreeModel* model_ {};
    CInfo& info_ {};
    CSettings& settings_ {};
};

#endif // TREEWIDGETSTAKEHOLDER_H
