#include "deletenode.h"
#include "ui_deletenode.h"

DeleteNode::DeleteNode(int node_id, const QMultiMap<QString, int>& leaf_map, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::DeleteNode)
    , node_id_ { node_id }
{
    ui->setupUi(this);
    IniDialog();
    IniConnect();

    QString leaf_path {};

    for (auto it = leaf_map.cbegin(); it != leaf_map.cend(); ++it) {
        QString path = it.key();
        int id = it.value();

        ui->comboBox->addItem(path, id);

        if (id == node_id)
            leaf_path = path;
    }

    setWindowTitle(QString("Delete Note : %1").arg(leaf_path));
}

DeleteNode::~DeleteNode()
{
    delete ui;
}

void DeleteNode::CustomAccept()
{
    if (ui->rBtnDeleteALl->isChecked()) {
        emit SendDelete(node_id_);
    } else {
        int new_node_id = ui->comboBox->itemData(ui->comboBox->currentIndex()).toInt();
        emit SendReplace(node_id_, new_node_id);
    }

    emit SendReloadAll(node_id_);
    accept();
}

void DeleteNode::IniDialog()
{
    ui->label_2->setWordWrap(true);
    ui->rBtnMoveTo->setChecked(true);
    ui->pBtnCancel->setDefault(true);
}

void DeleteNode::IniConnect()
{
    connect(ui->pBtnOk, &QPushButton::clicked, this, &DeleteNode::CustomAccept, Qt::UniqueConnection);
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject, Qt::UniqueConnection);
    connect(ui->rBtnMoveTo, &QRadioButton::toggled, ui->comboBox, &QComboBox::setEnabled);
}
