#ifndef LINKEDDOCUMENT_H
#define LINKEDDOCUMENT_H

#include "../table/transaction.h"
#include <QDialog>
#include <QStringListModel>

namespace Ui {
class LinkedDocument;
}

class LinkedDocument : public QDialog {
    Q_OBJECT

public:
    LinkedDocument(QSharedPointer<Transaction> transaction, const QString& document_dir, QWidget* parent = nullptr);
    ~LinkedDocument();

signals:
    void SendUpdate(const Transaction& transaction);
    void SendDocument(const QSharedPointer<Transaction>& transaction);

private slots:
    void on_pBtnAdd_clicked();
    void on_pBtnRemove_clicked();
    void CustomAccept();
    void on_listView_doubleClicked(const QModelIndex& index);

private:
    void CreateList(const QStringList& list);
    void IniDialog();
    void IniConnect();

private:
    Ui::LinkedDocument* ui;
    QSharedPointer<Transaction> transaction_ {};
    QStringListModel* list_model_ {};
    QString document_dir_ {};
};

#endif // LINKEDDOCUMENT_H
