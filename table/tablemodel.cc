#include "tablemodel.h"
#include "../globalmanager/sqlconnectionpool.h"
#include "../globalmanager/transactionpool.h"
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

TableModel::TableModel(const TableInfo& info, QObject* parent)
    : QAbstractItemModel { parent }
    , db_ { SqlConnectionPool::Instance().Allocate() }
    , table_info_ { info }
{

    header_ << "ID"
            << "Date"
            << "Description"
            << "Transfor"
            << "S"
            << "D"
            << "Debit"
            << "Credit"
            << "Balance";

    CreateTable(info.node_id, transaction_list_);
    AccumulateBalance(transaction_list_, 0, table_info_.node_rule);
}

TableModel::~TableModel()
{
    RemoveEmptyTransfer(transaction_list_);
    SqlConnectionPool::Instance().Recycle(db_);
    RecycleTransaction(transaction_list_);
}

void TableModel::ReceiveRule(int node_id, bool rule)
{
    if (table_info_.node_id != node_id || table_info_.node_rule == rule)
        return;

    table_info_.node_rule = rule;
    AccumulateBalance(transaction_list_, 0, table_info_.node_rule);
}

void TableModel::ReceiveUpdate(const Transaction& transaction)
{
    if (table_info_.node_id != transaction.transfer)
        return;

    int row = GetRow(transaction.id);
    if (row == -1)
        return;

    auto old_transaction = transaction_list_.at(row);

    bool des_changed = old_transaction->description != transaction.description;
    bool deb_cre_changed = old_transaction->debit != transaction.credit || old_transaction->credit != transaction.debit;
    bool pos_changed = old_transaction->post_date != transaction.post_date;
    bool sta_changed = old_transaction->status != transaction.status;
    bool doc_changed = old_transaction->document != transaction.document;

    if (des_changed)
        old_transaction->description = transaction.description;

    if (pos_changed)
        old_transaction->post_date = transaction.post_date;

    if (deb_cre_changed) {
        old_transaction->debit = transaction.credit;
        old_transaction->credit = transaction.debit;
        AccumulateBalance(transaction_list_, row, table_info_.node_rule);
    }

    if (sta_changed)
        old_transaction->status = transaction.status;

    if (doc_changed)
        old_transaction->document = transaction.document;
}

void TableModel::ReceiveRemove(int node_id, int trans_id)
{
    if (table_info_.node_id != node_id)
        return;

    int row = GetRow(trans_id);
    if (row == -1)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    auto transaction = transaction_list_.takeAt(row);
    endRemoveRows();

    AccumulateBalance(transaction_list_, row, table_info_.node_rule);

    TransactionPool::Instance().Recycle(transaction);
}

void TableModel::ReceiveCopy(int node_id, const Transaction& transaction)
{
    if (table_info_.node_id != transaction.transfer)
        return;

    auto new_transaction = TransactionPool::Instance().Allocate();
    new_transaction->post_date = transaction.post_date;
    new_transaction->id = transaction.id;
    new_transaction->description = transaction.description;
    new_transaction->debit = transaction.credit;
    new_transaction->credit = transaction.debit;
    new_transaction->transfer = node_id;

    int row = transaction_list_.size() - 1;

    beginInsertRows(QModelIndex(), row, row);
    transaction_list_.emplaceBack(new_transaction);
    endInsertRows();

    AccumulateBalance(transaction_list_, row, table_info_.node_rule);
}

void TableModel::ReceiveReload(int node_id)
{
    if (table_info_.node_id != node_id)
        return;

    RecycleTransaction(transaction_list_);

    beginResetModel();
    CreateTable(node_id, transaction_list_);
    endResetModel();
}

void TableModel::ReceiveDocument(const QSharedPointer<Transaction>& transaction)
{
    UpdateRecord(transaction);
}

int TableModel::GetRow(int trans_id) const
{
    int row { 0 };

    for (const auto& transaction : transaction_list_) {
        if (transaction->id == trans_id) {
            return row;
        }
        ++row;
    }
    return -1;
}

int TableModel::EmptyTransfer() const
{
    int row { 0 };

    for (const auto& transaction : transaction_list_) {
        if (transaction->transfer == 0) {
            return row;
        }
        ++row;
    }
    return -1;
}

QSharedPointer<Transaction> TableModel::GetTransaction(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;

    int row = index.row();
    if (row < 0 || row >= rowCount())
        return nullptr;

    return transaction_list_.at(row);
}

void TableModel::RecycleTransaction(QList<QSharedPointer<Transaction>>& list)
{
    for (auto& transaction : list) {
        TransactionPool::Instance().Recycle(transaction);
    }

    list.clear();
}

void TableModel::RemoveEmptyTransfer(QList<QSharedPointer<Transaction>>& list)
{
    int row = EmptyTransfer();
    if (row != -1) {
        int trans_id = list.at(row)->id;
        RemoveRecord(trans_id);
    }
}

QModelIndex TableModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex TableModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QModelIndex();
}

void TableModel::CreateTable(int node_id, QList<QSharedPointer<Transaction>>& list)
{
    QSqlQuery query_1st(db_);
    query_1st.setForwardOnly(true);
    // query.exec("PRAGMA foreign_keys = ON;");

    QSqlQuery query_2nd(db_);
    query_2nd.setForwardOnly(true);
    // query_2nd.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("SELECT id, credit, description, document, status, amount, post_date "
                                  "FROM %1 "
                                  "WHERE debit = :node_id")
                              .arg(table_info_.transaction_table);

    const auto part_2nd = QString("SELECT id, debit, description, document, status, amount, post_date "
                                  "FROM %1 "
                                  "WHERE credit = :node_id ")
                              .arg(table_info_.transaction_table);

    if (!DBTransaction([&]() {
            query_1st.prepare(part_1st);
            query_1st.bindValue(":node_id", node_id);

            if (!query_1st.exec()) {
                qWarning() << "Error in ConstructTable 1st" << query_1st.lastError().text();
                return false;
            }

            query_2nd.prepare(part_2nd);
            query_2nd.bindValue(":node_id", node_id);

            if (!query_2nd.exec()) {
                qWarning() << "Error in ConstructTable 2nd" << query_2nd.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed to create table";
        return;
    }

    while (query_1st.next()) {
        auto transaction = TransactionPool::Instance().Allocate();
        transaction->id = query_1st.value("id").toInt();
        transaction->transfer = query_1st.value("credit").toInt();
        transaction->debit = query_1st.value("amount").toDouble();
        transaction->description = query_1st.value("description").toString();
        transaction->document = query_1st.value("document").toString().split(";", Qt::SkipEmptyParts);
        transaction->post_date = query_1st.value("post_date").toDate();
        transaction->status = query_1st.value("status").toBool();
        list.emplace_back(transaction);
    }

    while (query_2nd.next()) {
        auto transaction = TransactionPool::Instance().Allocate();
        transaction->id = query_2nd.value("id").toInt();
        transaction->transfer = query_2nd.value("debit").toInt();
        transaction->credit = query_2nd.value("amount").toDouble();
        transaction->description = query_2nd.value("description").toString();
        transaction->document = query_2nd.value("document").toString().split(";", Qt::SkipEmptyParts);
        transaction->post_date = query_2nd.value("post_date").toDate();
        transaction->status = query_2nd.value("status").toBool();
        list.emplace_back(transaction);
    }

    std::sort(list.begin(), list.end(),
        [](const auto& lhs, const auto& rhs)
            -> bool { return lhs->post_date < rhs->post_date; });
}

int TableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return transaction_list_.size();
}

int TableModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return header_.size();
}

QVariant TableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    int row = index.row();
    int column = index.column();

    if (row >= 0 && row < transaction_list_.size()) {
        auto transaction = transaction_list_.at(row);
        switch (column) {
        case Table::kID:
            return transaction->id;
        case Table::kPostDate:
            return transaction->post_date;
        case Table::kDescription:
            return transaction->description;
        case Table::kTransfer:
            return transaction->transfer == 0 ? QVariant() : transaction->transfer;
        case Table::kStatus:
            return transaction->status;
        case Table::kDocument:
            return transaction->document.size() == 0 ? QVariant() : QString::number(transaction->document.size());
        case Table::kDebit:
            return transaction->debit == 0 ? QVariant() : QString::number(transaction->debit, 'f', table_info_.decimal);
        case Table::kCredit:
            return transaction->credit == 0 ? QVariant() : QString::number(transaction->credit, 'f', table_info_.decimal);
        case Table::kBalance:
            return QString::number(transaction->balance, 'f', table_info_.decimal);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

bool TableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    const int kRow = index.row();
    const int kColumn = index.column();

    if (kRow < 0 || kRow >= transaction_list_.size())
        return false;

    auto transaction = transaction_list_.at(kRow);
    int transfer_id = transaction->transfer;

    bool pos_changed { false };
    bool des_changed { false };
    bool tra_changed { false };
    bool deb_changed { false };
    bool cre_changed { false };
    bool sta_changed { false };

    switch (kColumn) {
    case Table::kPostDate:
        pos_changed = UpdatePostDate(transaction, value.toDate());
        break;
    case Table::kDescription:
        des_changed = UpdateDescription(transaction, value.toString());
        break;
    case Table::kTransfer:
        tra_changed = UpdateTransfer(transaction, value.toInt());
        break;
    case Table::kStatus:
        sta_changed = UpdateStatus(transaction, value.toBool());
        break;
    case Table::kDebit:
        deb_changed = UpdateDebit(transaction, value.toDouble());
        break;
    case Table::kCredit:
        cre_changed = UpdateCredit(transaction, value.toDouble());
        break;
    default:
        return false;
    }

    if (deb_changed || cre_changed) {
        AccumulateBalance(transaction_list_, kRow, table_info_.node_rule);
        emit SendUpdate(*transaction);
        UpdateRecord(transaction);
        emit SendReCalculate(table_info_.node_id);
        emit SendReCalculate(transfer_id);
    }

    if (tra_changed) {
        emit SendCopy(table_info_.node_id, *transaction);
        emit SendRemove(transfer_id, transaction->id);
        UpdateRecord(transaction);
        emit SendReCalculate(transaction->transfer);
        emit SendReCalculate(transfer_id);
    }

    if (des_changed || pos_changed || sta_changed) {
        emit SendUpdate(*transaction);
        UpdateRecord(transaction);
    }

    return true;
}

bool TableModel::UpdatePostDate(QSharedPointer<Transaction>& transaction, const QDate& value)
{
    if (transaction->post_date == value)
        return false;

    transaction->post_date = value;
    return true;
}

bool TableModel::UpdateDescription(QSharedPointer<Transaction>& transaction, const QString& value)
{
    if (transaction->description == value) {
        return false;
    }

    transaction->description = value;
    return true;
}

bool TableModel::UpdateTransfer(QSharedPointer<Transaction>& transaction, int new_node_id)
{
    if (new_node_id == transaction->transfer || new_node_id == table_info_.node_id)
        return false;

    transaction->transfer = new_node_id;
    return true;
}

bool TableModel::UpdateStatus(QSharedPointer<Transaction>& transaction, bool status)
{
    if (transaction->status == status)
        return false;

    transaction->status = status;
    return true;
}

bool TableModel::UpdateDebit(QSharedPointer<Transaction>& transaction, double value)
{
    double old_debit = transaction->debit;
    if (old_debit == value) {
        return false;
    }

    double old_credit = transaction->credit;
    double amount = qAbs(value - old_credit);

    transaction->debit = (value >= old_credit) ? amount : 0;
    transaction->credit = (value < old_credit) ? amount : 0;

    return true;
}

bool TableModel::UpdateCredit(QSharedPointer<Transaction>& transaction, double value)
{
    double old_credit = transaction->credit;
    if (old_credit == value) {
        return false;
    }

    double old_debit = transaction->debit;
    double amount = qAbs(value - old_debit);

    transaction->debit = (value >= old_debit) ? 0 : amount;
    transaction->credit = (value < old_debit) ? 0 : amount;

    return true;
}

double TableModel::CalculateBalance(bool rule, double debit, double credit)
{
    return rule ? credit - debit : debit - credit;
}

void TableModel::AccumulateBalance(QList<QSharedPointer<Transaction>>& list, int row, bool rule)
{
    if (row < 0 || row >= list.size() || list.isEmpty()) {
        return;
    }

    double previous_balance = (row > 0) ? list[row - 1]->balance : 0.0;

    std::accumulate(list.begin() + row, list.end(), previous_balance,
        [&](double balance, auto& transaction) {
            transaction->balance = CalculateBalance(rule, transaction->debit, transaction->credit) + balance;
            return transaction->balance;
        });
}

bool TableModel::UpdateRecord(const QSharedPointer<Transaction>& transaction)
{
    int debit_id = transaction->debit == 0 ? transaction->transfer : table_info_.node_id;
    int credit_id = transaction->debit == 0 ? table_info_.node_id : transaction->transfer;
    double amount = transaction->debit == 0 ? transaction->credit : transaction->debit;
    QString post_date = transaction->post_date.toString("yyyy-MM-dd");
    QString document = transaction->document.join(";");

    QSqlQuery query(db_);
    // query.exec("PRAGMA foreign_keys = ON;");

    const QString part = QString("UPDATE %1 SET "
                                 "description = :description, "
                                 "amount = :amount, "
                                 "debit = :debit, "
                                 "credit = :credit, "
                                 "status = :status, "
                                 "document = :document, "
                                 "post_date = :post_date "
                                 "WHERE id = :id ")
                             .arg(table_info_.transaction_table);

    query.prepare(part);
    query.bindValue(":id", transaction->id);
    query.bindValue(":description", transaction->description);
    query.bindValue(":amount", amount);
    query.bindValue(":debit", debit_id);
    query.bindValue(":credit", credit_id);
    query.bindValue(":post_date", post_date);
    query.bindValue(":status", transaction->status);
    query.bindValue(":document", document);

    if (!query.exec()) {
        qWarning() << "Failed to update record in transaction table " << query.lastError().text();
        return false;
    }

    return true;
}

QVariant TableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

void TableModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    if (column < 0 || column >= header_.size() - 1) {
        emit layoutChanged();
        return;
    }

    auto Compare = [column, order](const auto& lhs, const auto& rhs) -> bool {
        switch (column) {
        case Table::kPostDate:
            return (order == Qt::AscendingOrder) ? (lhs->post_date < rhs->post_date) : (lhs->post_date > rhs->post_date);
        case Table::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case Table::kTransfer:
            return (order == Qt::AscendingOrder) ? (lhs->transfer < rhs->transfer) : (lhs->transfer > rhs->transfer);
        case Table::kStatus:
            return (order == Qt::AscendingOrder) ? (lhs->status < rhs->status) : (lhs->status > rhs->status);
        case Table::kDocument:
            return (order == Qt::AscendingOrder) ? (lhs->document.size() < rhs->document.size()) : (lhs->document.size() > rhs->document.size());
        case Table::kDebit:
            return (order == Qt::AscendingOrder) ? (lhs->debit < rhs->debit) : (lhs->debit > rhs->debit);
        case Table::kCredit:
            return (order == Qt::AscendingOrder) ? (lhs->credit < rhs->credit) : (lhs->credit > rhs->credit);
        default:
            return false;
        }
    };

    std::sort(transaction_list_.begin(), transaction_list_.end(), Compare);
    AccumulateBalance(transaction_list_, 0, table_info_.node_rule);
    emit layoutChanged();
}

bool TableModel::insertRow(int row, const QModelIndex& parent)
{
    if (row < 0)
        return false;

    auto transaction = TransactionPool::Instance().Allocate();

    beginInsertRows(parent, row, row);
    transaction_list_.insert(row, transaction);
    endInsertRows();

    AccumulateBalance(transaction_list_, row, table_info_.node_rule);
    InsertRecord(table_info_.node_id, transaction);

    return true;
}

bool TableModel::InsertRecord(int node_id, QSharedPointer<Transaction>& transaction)
{
    QSqlQuery query(db_);
    const auto part = QString("INSERT INTO %1 (debit) VALUES (:node_id) ")
                          .arg(table_info_.transaction_table);

    query.prepare(part);
    query.bindValue(":node_id", node_id);
    if (!query.exec()) {
        qWarning() << "Failed to insert record in transaction table" << query.lastError().text();
        return false;
    }

    transaction->id = query.lastInsertId().toInt();

    return true;
}

bool TableModel::removeRow(int row, const QModelIndex& parent)
{
    if (row < 0)
        return false;

    auto transaction = transaction_list_.at(row);
    int id = transaction->id;
    int node_transfer_id = transaction->transfer;
    int node_id = table_info_.node_id;

    beginRemoveRows(parent, row, row);
    transaction_list_.removeAt(row);
    endRemoveRows();

    emit SendRemove(node_transfer_id, id);

    if (!transaction_list_.isEmpty())
        AccumulateBalance(transaction_list_, row, table_info_.node_rule);

    TransactionPool::Instance().Recycle(transaction);

    RemoveRecord(id);
    emit SendReCalculate(node_id);
    emit SendReCalculate(node_transfer_id);
    return true;
}

bool TableModel::RemoveRecord(int trans_id)
{
    QSqlQuery query(db_);
    const auto query_1st = QString("DELETE FROM %1 WHERE id = :trans_id ").arg(table_info_.transaction_table);

    query.prepare(query_1st);
    query.bindValue(":trans_id", trans_id);
    if (!query.exec()) {
        qWarning() << "Failed to remove record in transaction table" << query.lastError().text();
        return false;
    }

    return true;
}

bool TableModel::DBTransaction(auto Function)
{
    if (db_.transaction() && Function() && db_.commit()) {
        return true;
    } else {
        db_.rollback();
        qWarning() << "Transaction failed";
        return false;
    }
}

Qt::ItemFlags TableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags = QAbstractItemModel::flags(index);
    const int kColumn = index.column();

    switch (kColumn) {
    case Table::kID:
        flags &= ~Qt::ItemIsEditable;
        break;
    case Table::kBalance:
        flags &= ~Qt::ItemIsEditable;
        break;
    default:
        flags |= Qt::ItemIsEditable;
        break;
    }

    return flags;
}
