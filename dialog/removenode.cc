#include "removenode.h"

#include <QCompleter>
#include <QMessageBox>

#include "dialog/signalblocker.h"
#include "ui_removenode.h"

RemoveNode::RemoveNode(CTreeModel* model, Section section, int node_id, int unit, bool branch, bool exteral_reference, bool is_helper, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::RemoveNode)
    , node_id_ { node_id }
    , unit_ { unit }
    , branch_ { branch }
    , is_helper_ { is_helper }
    , section_ { section }
    , model_ { model }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniData(section, exteral_reference, is_helper);
    IniConnect();
}

RemoveNode::~RemoveNode() { delete ui; }

void RemoveNode::DisableRemove() { }

void RemoveNode::RCustomAccept()
{
    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    const auto& path { model_->GetPath(node_id_) };
    QString text {};
    QString informative_text {};

    if (ui->rBtnRemoveRecords->isChecked()) {
        text = tr("Remove Node");
        informative_text = tr("Remove %1 and all its internal references. Are you sure").arg(path);
    }

    if (ui->rBtnReplaceRecords->isChecked()) {
        text = tr("Replace Node");
        informative_text = tr("Replace %1 with %2 for all internal and external references. This action is risky and cannot be undone. Are you sure?")
                               .arg(path, ui->comboBox->currentText());
    }

    msg.setText(text);
    msg.setInformativeText(informative_text);

    if (msg.exec() == QMessageBox::Ok) {
        if (ui->rBtnRemoveRecords->isChecked())
            emit SRemoveNode(node_id_, branch_, is_helper_);

        if (ui->rBtnReplaceRecords->isChecked()) {
            int new_node_id { ui->comboBox->currentData().toInt() };
            emit SReplaceNode(node_id_, new_node_id, is_helper_);
        }

        accept();
    }
}

void RemoveNode::IniData(Section section, bool exteral_reference, bool is_helper)
{
    ui->label->setWordWrap(true);
    ui->pBtnCancel->setDefault(true);
    this->setWindowTitle(tr("Remove %1").arg(model_->GetPath(node_id_)));

    if (section == Section::kSales || section == Section::kPurchase) {
        ui->comboBox->setEnabled(false);
        ui->rBtnReplaceRecords->setEnabled(false);
        ui->rBtnRemoveRecords->setChecked(true);
        return;
    }

    ui->rBtnReplaceRecords->setChecked(true);

    if (exteral_reference) {
        ui->rBtnRemoveRecords->setDisabled(true);
        ui->label->setText(tr("The node has external references, so it can’t be removed directly. Should it be replaced instead?"));
    }

    // 不需要接收更新combo model的信号
    auto* combo_model_ { new QStandardItemModel(this) };

    if (is_helper) {
        model_->LeafPathHelperNodeFTS(combo_model_, node_id_, Filter::kExcludeSpecific);
    } else {
        model_->LeafPathRemoveNodeFPTS(combo_model_, unit_, node_id_);
    }

    ui->comboBox->setModel(combo_model_);
}

void RemoveNode::IniConnect()
{
    connect(ui->pBtnOk, &QPushButton::clicked, this, &RemoveNode::RCustomAccept);
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->rBtnReplaceRecords, &QRadioButton::toggled, ui->comboBox, &QComboBox::setEnabled);
}
