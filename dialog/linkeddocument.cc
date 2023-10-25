#include "linkeddocument.h"
#include "ui_linkeddocument.h"
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QUrl>

LinkedDocument::LinkedDocument(QSharedPointer<Transaction> transaction, const QString& home_dir, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::LinkedDocument)
    , transaction_ { transaction }
{
    ui->setupUi(this);
    CreateList(transaction->document);
    IniDialog();
    IniConnect();
    document_dir_ = home_dir;
}

LinkedDocument::~LinkedDocument()
{
    delete list_model_;
    delete ui;
}

void LinkedDocument::CreateList(const QStringList& list)
{
    list_model_ = new QStringListModel(this);
    list_model_->setStringList(list);
    ui->listView->setModel(list_model_);
}

void LinkedDocument::IniDialog()
{
    ui->pBtnOk->setDefault(true);
    ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    setWindowTitle("Linked Document");
}

void LinkedDocument::IniConnect()
{
    connect(ui->pBtnCancel, &QPushButton::clicked, this, &QDialog::reject, Qt::UniqueConnection);
    connect(ui->pBtnOk, &QPushButton::clicked, this, &LinkedDocument::CustomAccept, Qt::UniqueConnection);
}

void LinkedDocument::on_pBtnAdd_clicked()
{
    QString filter("*.pdf");
    auto added_list = QFileDialog::getOpenFileNames(this, "Select Original Document", document_dir_, filter, nullptr, QFileDialog::ReadOnly);
    auto destination_list = list_model_->stringList();

    for (const auto& path : added_list) {
        if (!destination_list.contains(path)) {
            int index = list_model_->rowCount();
            list_model_->insertRow(index);
            list_model_->setData(list_model_->index(index), path);
        }
    }
}

void LinkedDocument::on_pBtnRemove_clicked()
{
    auto index = ui->listView->currentIndex();
    list_model_->removeRow(index.row(), QModelIndex());
}

void LinkedDocument::CustomAccept()
{
    transaction_->document = list_model_->stringList();
    emit SendUpdate(*transaction_);
    emit SendDocument(transaction_);
    accept();
}

void LinkedDocument::on_listView_doubleClicked(const QModelIndex& index)
{
    auto path = index.data().toString();
    auto file_url = QUrl::fromLocalFile(path);

    if (QFile::exists(path)) {
        QDesktopServices::openUrl(file_url);
    } else {
        qDebug() << "File does not exist: " << path;
    }
}
