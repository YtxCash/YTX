#include "removenode.h"

#include <QCompleter>
#include <QMessageBox>

#include "dialog/signalblocker.h"
#include "ui_removenode.h"

RemoveNode::RemoveNode(CTreeModel* model, int node_id, int unit, bool disable, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::RemoveNode)
    , node_id_ { node_id }
    , unit_ { unit }
    , model_ { model }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog();
    IniConnect();

    if (disable)
        DisableRemove();
}

RemoveNode::~RemoveNode() { delete ui; }

void RemoveNode::DisableRemove()
{
    ui->rBtnRemoveRecords->setDisabled(true);
    ui->label->setText(tr("The node has external references, so it can’t be removed directly. Should it be replaced instead?"));
}

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
            emit SRemoveNode(node_id_);

        if (ui->rBtnReplaceRecords->isChecked()) {
            int new_node_id { ui->comboBox->currentData().toInt() };
            emit SReplaceNode(node_id_, new_node_id);
        }

        accept();
    }
}

void RemoveNode::IniDialog()
{
    ui->label->setWordWrap(true);
    ui->rBtnReplaceRecords->setChecked(true);
    ui->pBtnCancel->setDefault(true);
    this->setWindowTitle(tr("Remove %1").arg(model_->GetPath(node_id_)));

    // 不需要接收更新combo model的信号
    auto* combo_model_ { new QStandardItemModel(this) };
    model_->LeafPathExcludeIDFPTS(combo_model_, node_id_);
    ui->comboBox->setModel(combo_model_);
}

void RemoveNode::IniConnect()
{
    connect(ui->pBtnOk, &QPushButton::clicked, this, &RemoveNode::RCustomAccept);
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->rBtnReplaceRecords, &QRadioButton::toggled, ui->comboBox, &QComboBox::setEnabled);
}
