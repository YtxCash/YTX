#include "insertnode.h"
#include "ui_insertnode.h"

InsertNode::InsertNode(Node* node, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::InsertNode)
    , node_ { node }
{
    ui->setupUi(this);
    IniDialog();
    IniConnect();

    if (node->rule)
        ui->rBtnDDCI->setChecked(true);
    else
        ui->rBtnDICD->setChecked(true);
}

InsertNode::~InsertNode()
{
    delete ui;
}

void InsertNode::IniDialog()
{
    ui->labNote->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    ui->gridLayout->setContentsMargins(0, 0, 0, 0);
    ui->pBtnOk->setDefault(true);
    ui->pBtnOk->setShortcut(QKeySequence(Qt::Key_Enter));
}

void InsertNode::IniConnect()
{
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject, Qt::UniqueConnection);
    connect(ui->lineEditName, &QLineEdit::textChanged, this, &InsertNode::SetWindowTitle, Qt::UniqueConnection);
    connect(ui->pBtnOk, &QPushButton::clicked, this, &InsertNode::CustomAccept, Qt::UniqueConnection);
}

void InsertNode::SetData()
{
    node_->name = ui->lineEditName->text().trimmed();
    node_->description = ui->lineEditDescription->text().simplified();
    node_->placeholder = ui->chkBoxPlaceholer->isChecked();
    node_->rule = ui->rBtnDICD->isChecked() ? false : true;
}

void InsertNode::CustomAccept()
{
    SetData();
    accept();
}

void InsertNode::SetWindowTitle(const QString& name)
{
    setWindowTitle(name);
}
