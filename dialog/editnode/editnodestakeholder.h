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

#ifndef EDITNODESTAKEHOLDER_H
#define EDITNODESTAKEHOLDER_H

#include <QComboBox>
#include <QDialog>

#include "component/using.h"
#include "tree/model/treemodel.h"
#include "tree/node.h"

namespace Ui {
class EditNodeStakeholder;
}

class EditNodeStakeholder final : public QDialog {
    Q_OBJECT

public:
    EditNodeStakeholder(Node* node, CStringMap& unit_map, CString& parent_path, CStringList& name_list, bool branch_enable, bool unit_enable,
        int amount_decimal, TreeModel* stakeholder_tree, QWidget* parent = nullptr);
    ~EditNodeStakeholder();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineEditName_editingFinished();
    void on_lineEditCode_editingFinished();
    void on_lineEditDescription_editingFinished();
    void on_dSpinPaymentPeriod_editingFinished();
    void on_dSpinTaxRate_editingFinished();

    void on_chkBoxBranch_toggled(bool checked);
    void on_rBtnMonthly_toggled(bool checked);

    void on_comboUnit_currentIndexChanged(int index);
    void on_comboEmployee_currentIndexChanged(int index);

    void on_plainTextEdit_textChanged();
    void on_deadline_editingFinished();

private:
    void IniDialog(CStringMap& unit_map, TreeModel* stakeholder_tree, int common_decimal);
    void IniComboWithStringMap(QComboBox* combo, CStringMap& map);
    void IniComboEmployee(TreeModel* stakeholder_tree);
    void IniConnect();
    void Data(Node* node, bool branch_enable, bool unit_enable);

private:
    Ui::EditNodeStakeholder* ui;
    Node* node_ {};

    QString parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODESTAKEHOLDER_H
