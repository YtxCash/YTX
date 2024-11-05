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

#ifndef EDITNODEFIANNCE_H
#define EDITNODEFIANNCE_H

#include <QDialog>

#include "component/using.h"
#include "tree/node.h"

namespace Ui {
class EditNodeFinance;
}

class EditNodeFinance final : public QDialog {
    Q_OBJECT

public:
    EditNodeFinance(
        Node* node, CStringHash& unit_hash, CString& parent_path, CStringList& name_list, bool branch_enable, bool unit_enable, QWidget* parent = nullptr);
    ~EditNodeFinance();

private slots:
    void RNameEdited(const QString& arg1);

    void on_lineName_editingFinished();
    void on_lineCode_editingFinished();
    void on_lineDescription_editingFinished();

    void on_comboUnit_currentIndexChanged(int index);

    void on_rBtnDDCI_toggled(bool checked);
    void on_chkBoxBranch_toggled(bool checked);

    void on_plainNote_textChanged();

private:
    void IniDialog(CStringHash& unit_hash);
    void IniConnect();
    void Data(Node* node, bool branch_enable, bool unit_enable);

private:
    Ui::EditNodeFinance* ui;
    Node* node_ {};

    CString& parent_path_ {};
    CStringList& name_list_ {};
};

#endif // EDITNODEFIANNCE_H
