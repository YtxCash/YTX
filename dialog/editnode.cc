#include "editnode.h"
#include "ui_editnode.h"
#include <QMessageBox>

EditNode::EditNode(Node* node, bool usage, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNode)
    , node_ { node }
    , usage_ { usage }
{
    ui->setupUi(this);
    IniDialog();
    IniConnect();
    Data(node);
}

EditNode::~EditNode()
{
    delete ui;
}

void EditNode::IniDialog()
{
    ui->labNote->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->pBtnOk->setDefault(true);
    ui->pBtnOk->setShortcut(QKeySequence(Qt::Key_Enter));
}

void EditNode::IniConnect()
{
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject, Qt::UniqueConnection);
    connect(ui->pBtnOk, &QPushButton::clicked, this, &EditNode::CustomAccept, Qt::UniqueConnection);
}

void EditNode::Data(Node* node)
{
    ui->lineEditName->setText(node->name);
    ui->lineEditDescription->setText(node->description);
    ui->chkBoxPlaceholer->setChecked(node->placeholder);
    setWindowTitle(QString("Edit Node : %1").arg(node->name));

    if (node->rule)
        ui->rBtnDDCI->setChecked(true);
    else
        ui->rBtnDICD->setChecked(true);
}

void EditNode::SetData()
{
    node_->name = ui->lineEditName->text().trimmed();
    node_->description = ui->lineEditDescription->text().simplified();
    node_->placeholder = ui->chkBoxPlaceholer->isChecked();

    const bool DICD = ui->rBtnDICD->isChecked() ? false : true;
    if (node_->rule != DICD) {
        node_->rule = DICD;
        node_->total = -node_->total;
    }
}

void EditNode::CustomAccept()
{
    if (usage_ && ui->chkBoxPlaceholer->isChecked()) {
        QMessageBox msg_box;
        msg_box.setIcon(QMessageBox::Warning);
        msg_box.setText("Node usage");
        msg_box.setInformativeText("The node has at least one transaction that cannot be set as a placeholder");
        msg_box.exec();
        ui->chkBoxPlaceholer->setChecked(false);
    } else {
        SetData();
        accept();
        emit SendUpdate(node_);
    }
}
