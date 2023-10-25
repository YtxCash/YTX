#include "treemodel.h"
#include "../globalmanager/nodepool.h"
#include "../globalmanager/sqlconnectionpool.h"
#include <QDebug>
#include <QIODevice>
#include <QMimeData>
#include <QQueue>
#include <QSqlError>
#include <QSqlQuery>

TreeModel::TreeModel(const TreeInfo& tree_info, QObject* parent)
    : QAbstractItemModel { parent }
    , tree_info_ { tree_info }
{
    header_ << "Account"
            << "Id"
            << "Description"
            << "Rule"
            << "Placeholder"
            << "Total";

    root_ = new Node(-1, "root", "", 0.0, false, true);
    db_ = SqlConnectionPool::Instance().Allocate();
    CreateTree(node_hash_, leaf_map_);
}

TreeModel::~TreeModel()
{
    delete root_;
    SqlConnectionPool::Instance().Recycle(db_);
}

void TreeModel::ReceiveReCalculate(int node_id)
{
    if (node_id == 0)
        return;

    auto node = GetNode(node_id);
    if (!node)
        return;

    double old_total = node->total;
    node->total = CalculateLeafTotal(node_id, node->rule);
    double difference = node->total - old_total;

    CalculatePlaceholderTotal(node, difference);
}

void TreeModel::ReceiveUpdate(const Node* node)
{
    UpdateRecord(node);
}

bool TreeModel::ReceiveDelete(int node_id)
{
    QSqlQuery query(db_);
    const auto part = QString("DELETE FROM %1 WHERE (debit = :node_id OR credit = :node_id) ").arg(tree_info_.node_transaction_table);

    query.prepare(part);
    query.bindValue(":node_id", node_id);
    if (!query.exec()) {
        qWarning() << "Failed to delete record in transaction table" << query.lastError().text();
        return false;
    }

    return true;
}

bool TreeModel::ReceiveReplace(int old_node_id, int new_node_id)
{
    QSqlQuery query(db_);
    const auto part = QString("UPDATE %1 SET "
                              "debit = CASE WHEN debit = :old_node_id THEN :new_node_id ELSE debit END, "
                              "credit = CASE WHEN credit = :old_node_id THEN :new_node_id ELSE credit END ")
                          .arg(tree_info_.node_transaction_table);

    query.prepare(part);
    query.bindValue(":new_node_id", new_node_id);
    query.bindValue(":old_node_id", old_node_id);
    if (!query.exec()) {
        qWarning() << "Error in replace setp" << query.lastError().text();
        return false;
    }

    return true;
}

void TreeModel::CreateTree(QHash<int, Node*>& node_hash, QMultiMap<QString, int>& leaf_map)
{
    QSqlQuery query_1st(db_);
    query_1st.setForwardOnly(true);
    // query_1st.exec("PRAGMA foreign_keys = ON;");

    QSqlQuery query_2nd(db_);
    query_2nd.setForwardOnly(true);
    // query_2nd.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("SELECT "
                                  "F1.id AS ancestor_id, "
                                  "F1.name AS ancestor_name, "
                                  "F1.description AS ancestor_description, "
                                  "F1.rule AS ancestor_rule, "
                                  "F1.placeholder AS ancestor_placeholder, "
                                  "F2.id AS descendant_id, "
                                  "F2.name AS descendant_name, "
                                  "F2.description AS descendant_description, "
                                  "F2.rule AS descendant_rule, "
                                  "F2.placeholder AS descendant_placeholder "
                                  "FROM %2 FP "
                                  "INNER JOIN %1 F1 ON FP.ancestor = F1.id "
                                  "INNER JOIN %1 F2 ON FP.descendant = F2.id "
                                  "WHERE FP.distance = 1 ")
                              .arg(tree_info_.node_table, tree_info_.node_path_table);

    const auto part_2nd = QString("SELECT fp.descendant, f.name, f.description, f.rule, f.placeholder "
                                  "FROM %2 fp "
                                  "JOIN %1 f ON fp.descendant = f.id "
                                  "GROUP BY fp.descendant, f.name "
                                  "HAVING COUNT(fp.descendant) = 1 ")
                              .arg(tree_info_.node_table, tree_info_.node_path_table);

    if (!DBTransaction([&]() {
            query_1st.prepare(part_1st);
            if (!query_1st.exec()) {
                qWarning() << "Error in create tree 1 setp " << query_1st.lastError().text();
                return false;
            }

            query_2nd.prepare(part_2nd);
            if (!query_2nd.exec()) {
                qWarning() << "Error in create tree 2 setp " << query_2nd.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed in create tree ";
        return;
    }

    CreateNodeHash(query_1st, node_hash);
    AddIsolatedNode(query_2nd, node_hash);

    for (auto node : qAsConst(node_hash)) {
        if (!node->parent) {
            node->parent = root_;
            root_->children.emplace_back(node);
        }
    }

    for (auto it = node_hash.begin(); it != node_hash.end(); ++it) {
        if (!it.value()->placeholder) {
            int id = it.key();
            auto node = it.value();
            bool rule = node->rule;

            node->total = CalculateLeafTotal(id, rule);
            CalculatePlaceholderTotal(node, node->total);

            auto path = CreatePathForLeaf(node);
            leaf_map.insert(path, id);
        }
    }
}

void TreeModel::CreatePathForAll(const QHash<int, Node*>& node_hash, QMultiMap<QString, int>& leaf_map)
{
    leaf_map.clear();

    for (auto it = node_hash.cbegin(); it != node_hash.cend(); ++it) {
        if (!it.value()->placeholder) {
            int id = it.key();
            auto node = it.value();

            auto path = CreatePathForLeaf(node);
            leaf_map.insert(path, id);
        }
    }
}

QString TreeModel::CreatePathForLeaf(const Node* node)
{
    QStringList tmp { node->name };

    while (node->parent != root_) {
        node = node->parent;
        tmp.prepend(node->name);
    }

    return tmp.join("-");
}

double TreeModel::CalculateLeafTotal(int id, bool rule)
{
    QSqlQuery query_1st(db_);
    query_1st.setForwardOnly(true);
    // query_1st.exec("PRAGMA foreign_keys = ON;");

    QSqlQuery query_2nd(db_);
    query_2nd.setForwardOnly(true);
    // query_2nd.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("SELECT ft.amount FROM %1 ft "
                                  "WHERE debit = (:id) ")
                              .arg(tree_info_.node_transaction_table);

    const auto part_2nd = QString("SELECT ft.amount FROM %1 ft "
                                  "WHERE credit = (:id) ")
                              .arg(tree_info_.node_transaction_table);

    if (!DBTransaction([&]() {
            query_1st.prepare(part_1st);
            query_1st.bindValue(":id", id);
            if (!query_1st.exec()) {
                qWarning() << "Error in create tree 1 setp " << query_1st.lastError().text();
                return false;
            }

            query_2nd.prepare(part_2nd);
            query_2nd.bindValue(":id", id);
            if (!query_2nd.exec()) {
                qWarning() << "Error in create tree 2 setp " << query_2nd.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed in create tree ";
    }

    double total_debit { 0.0 };
    double total_credit { 0.0 };

    while (query_1st.next()) {
        total_debit += query_1st.value(0).toDouble();
    }

    while (query_2nd.next()) {
        total_credit += query_2nd.value(0).toDouble();
    }

    return rule ? total_credit - total_debit : total_debit - total_credit;
}

void TreeModel::CalculatePlaceholderTotal(const Node* node, double value)
{
    const bool kEqualRule = node->parent->rule == node->rule;
    double change = kEqualRule ? value : -value;

    while (node->parent != root_) {
        node->parent->total += change;
        node = node->parent;
    }
}

void TreeModel::RecalculateAllTotal(QHash<int, Node*>& node_hash)
{
    for (auto it = node_hash.begin(); it != node_hash.end(); ++it) {
        if (it.value()->placeholder)
            it.value()->total = 0.0;
    }

    for (auto it = node_hash.begin(); it != node_hash.end(); ++it) {
        if (!it.value()->placeholder) {
            int id = it.key();
            auto node = it.value();
            bool rule = node->rule;

            node->total = CalculateLeafTotal(id, rule);
            CalculatePlaceholderTotal(node, node->total);
        }
    }
}

bool TreeModel::UsageOfNode(int node_id) const
{
    QSqlQuery query(db_);
    query.setForwardOnly(true);
    // query.exec("PRAGMA foreign_keys = ON;");

    const auto string = QString("SELECT COUNT(*) FROM %1 WHERE debit = :node_id OR credit = :node_id ").arg(tree_info_.node_transaction_table);
    query.prepare(string);
    query.bindValue(":node_id", node_id);

    if (query.exec() && query.next()) {
        int row = query.value(0).toInt();
        if (row > 0)
            return true;
        else
            return false;
    } else {
        qWarning() << "Failed to count times " << query.lastError().text();
        return false;
    }
}

void TreeModel::CreateNodeHash(QSqlQuery& query, QHash<int, Node*>& node_hash)
{
    while (query.next()) {
        int ancestor_id = query.value("ancestor_id").toInt();
        int descendant_id = query.value("descendant_id").toInt();

        if (!node_hash.contains(ancestor_id)) {
            auto ancestor = NodePool::Instance().Allocate();
            ancestor->id = ancestor_id;
            ancestor->name = query.value("ancestor_name").toString();
            ancestor->description = query.value("ancestor_description").toString();
            ancestor->rule = query.value("ancestor_rule").toBool();
            ancestor->placeholder = query.value("ancestor_placeholder").toBool();

            node_hash.insert(ancestor_id, ancestor);
        }

        if (!node_hash.contains(descendant_id)) {
            auto descendant = NodePool::Instance().Allocate();
            descendant->id = descendant_id;
            descendant->name = query.value("descendant_name").toString();
            descendant->description = query.value("descendant_description").toString();
            descendant->rule = query.value("descendant_rule").toBool();
            descendant->placeholder = query.value("descendant_placeholder").toBool();

            node_hash.insert(descendant_id, descendant);
        }

        auto ancestor = node_hash[ancestor_id];
        auto descendant = node_hash[descendant_id];

        ancestor->children.emplaceBack(descendant);
        descendant->parent = ancestor;
    }
}

void TreeModel::AddIsolatedNode(QSqlQuery& query, QHash<int, Node*>& node_hash)
{
    while (query.next()) {
        int descendant_id = query.value("descendant").toInt();
        QString name = query.value("name").toString();
        QString description = query.value("description").toString();
        bool rule = query.value("rule").toBool();
        bool placeholder = query.value("placeholder").toBool();

        if (!node_hash.contains(descendant_id)) {
            auto descendant = NodePool::Instance().Allocate();
            descendant->id = descendant_id;
            descendant->name = name;
            descendant->description = description;
            descendant->rule = rule;
            descendant->placeholder = placeholder;

            node_hash.insert(descendant_id, descendant);
        }
    }
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    auto parent_node = GetNode(parent);

    if (row < 0 || row >= parent_node->children.size())
        return QModelIndex();

    auto node = parent_node->children.at(row);
    if (node)
        return createIndex(row, column, node);

    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    auto node = GetNode(index);
    if (node == root_)
        return QModelIndex();

    auto parent_node = node->parent;
    if (parent_node == root_)
        return QModelIndex();

    return createIndex(parent_node->parent->children.indexOf(parent_node), 0,
        parent_node);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
    auto parent_node = GetNode(parent);

    return parent_node->children.size();
}

QVariant TreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    auto node = GetNode(index);
    if (node == root_)
        return QVariant();

    const int kColumn = index.column();

    switch (kColumn) {
    case Tree::kName:
        return node->name;
    case Tree::kID:
        return node->id;
    case Tree::kDescription:
        return node->description;
    case Tree::kRule:
        return node->rule;
    case Tree::kPlaceholder:
        return node->placeholder;
    case Tree::kTotal:
        return QString::number(node->total, 'f', 2);
    default:
        return QVariant();
    }
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole)
        return false;

    auto node = GetNode(index);
    if (node == root_) {
        return false;
    }

    const int kColumn = index.column();

    bool pla_changed { false };
    bool des_changed { false };
    bool rul_changed { false };

    switch (kColumn) {
    case Tree::kDescription:
        des_changed = UpdateDescription(node, value.toString());
        break;
    case Tree::kRule:
        rul_changed = UpdateRule(node, value.toBool());
        break;
    case Tree::kPlaceholder:
        pla_changed = UpdatePlaceholder(node, value.toBool());
        break;
    default:
        return false;
    }

    if (pla_changed) {
        CreatePathForAll(node_hash_, leaf_map_);
        emit SendLeaf(leaf_map_);
    }

    if (rul_changed) {
        node->total = -node->total;
        if (!node->placeholder) {
            emit SendRule(node->id, node->rule);
        }
    }

    if (pla_changed || des_changed || rul_changed) {
        UpdateRecord(node);
    }

    return true;
}

bool TreeModel::UpdatePlaceholder(Node* node, const bool& value)
{
    if (node->placeholder == value || !node->children.isEmpty() || UsageOfNode(node->id))
        return false;

    node->placeholder = value;
    return true;
}

bool TreeModel::UpdateDescription(Node* node, const QString& value)
{
    if (node->description == value)
        return false;

    node->description = value;
    return true;
}

bool TreeModel::UpdateRule(Node* node, const bool& value)
{
    if (node->rule == value)
        return false;

    node->rule = value;
    return true;
}

int TreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return header_.size();
}

bool TreeModel::UpdateRecord(const Node* node)
{
    QSqlQuery query(db_);
    // query.exec("PRAGMA foreign_keys = ON;");

    int id = node->id;
    bool rule = node->rule;
    bool placeholder = node->placeholder;
    QString description = node->description;
    QString name = node->name;

    const auto part = QString("UPDATE %1 SET "
                              "name = :name, "
                              "description = :description, "
                              "placeholder = :placeholder, "
                              "rule = :rule "
                              "WHERE id = :id ")
                          .arg(tree_info_.node_table);

    query.prepare(part);
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":description", description);
    query.bindValue(":placeholder", placeholder);
    query.bindValue(":rule", rule);

    if (!query.exec()) {
        qWarning() << "Failed to update record: " << query.lastError().text();
        return false;
    }

    return true;
}

void TreeModel::sort(int column, Qt::SortOrder order)
{
    emit layoutAboutToBeChanged();

    if (column < 0 || column >= header_.size()) {
        emit layoutChanged();
        return;
    }

    auto Compare = [column, order](const auto lhs, const auto rhs) -> bool {
        switch (column) {
        case Tree::kName:
            return (order == Qt::AscendingOrder) ? (lhs->name < rhs->name) : (lhs->name > rhs->name);
        case Tree::kDescription:
            return (order == Qt::AscendingOrder) ? (lhs->description < rhs->description) : (lhs->description > rhs->description);
        case Tree::kRule:
            return (order == Qt::AscendingOrder) ? (lhs->rule < rhs->rule) : (lhs->rule > rhs->rule);
        case Tree::kPlaceholder:
            return (order == Qt::AscendingOrder) ? (lhs->placeholder < rhs->placeholder) : (lhs->placeholder > rhs->placeholder);
        case Tree::kTotal:
            return (order == Qt::AscendingOrder) ? (lhs->total < rhs->total) : (lhs->total > rhs->total);
        default:
            return false;
        }
    };

    SortIterative(root_, Compare);

    emit layoutChanged();
}

void TreeModel::SortIterative(Node* node, const auto& Compare)
{
    QQueue<Node*> queue {};
    queue.enqueue(node);

    while (!queue.isEmpty()) {
        auto node = queue.dequeue();

        if (!node->children.isEmpty()) {
            std::sort(node->children.begin(), node->children.end(), Compare);

            for (auto child : node->children) {
                queue.enqueue(child);
            }
        }
    }
}

Node* TreeModel::GetNode(const QModelIndex& index) const
{
    if (index.isValid() && index.internalPointer()) {
        return static_cast<Node*>(index.internalPointer());
    } else {
        return root_;
    }
}

Node* TreeModel::GetNode(int node_id) const
{
    if (node_hash_.contains(node_id))
        return node_hash_.value(node_id);

    return nullptr;
}

QMultiMap<QString, int> TreeModel::GetLeafMap() const
{
    return leaf_map_;
}

QString TreeModel::GetLeaf(int node_id) const
{
    for (auto it = leaf_map_.cbegin(); it != leaf_map_.cend(); ++it) {
        if (it.value() == node_id)
            return it.key();
    }

    return QString();
}

bool TreeModel::IsDescendant(Node* lhs, Node* rhs) const
{
    if (!lhs || !rhs) {
        return false;
    }

    if (lhs == rhs) {
        return false;
    }

    while (lhs && lhs != rhs) {
        lhs = lhs->parent;
    }

    return lhs == rhs;
}

bool TreeModel::insertRow(int row, const QModelIndex& parent, Node* node)
{
    auto parent_node = GetNode(parent);

    beginInsertRows(parent, row, row);
    node->parent = parent_node;
    parent_node->children.insert(row, node);
    endInsertRows();

    InsertRecord(parent_node->id, node);
    node_hash_.insert(node->id, node);

    if (node->placeholder)
        return true;

    QString path = CreatePathForLeaf(node);
    leaf_map_.insert(path, node->id);
    emit SendLeaf(leaf_map_);

    return true;
}

bool TreeModel::InsertRecord(int parent_id, Node* node)
{
    QSqlQuery query(db_);
    // query.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("INSERT INTO %1 (name, description, rule, placeholder) "
                                  "VALUES (:name, :description, :rule, :placeholder) ")
                              .arg(tree_info_.node_table);
    const auto part_2nd = QString("INSERT INTO %1 (ancestor, descendant, distance) "
                                  "SELECT ancestor, :id, distance + 1 FROM %1 "
                                  "WHERE descendant = :parent UNION ALL SELECT :id, :id, 0 ")
                              .arg(tree_info_.node_path_table);

    if (!DBTransaction([&]() {
            // 插入节点记录
            query.prepare(part_1st);
            query.bindValue(":name", node->name);
            query.bindValue(":description", node->description);
            query.bindValue(":rule", node->rule);
            query.bindValue(":placeholder", node->placeholder);

            if (!query.exec()) {
                qWarning() << "Failed to insert node record: " << query.lastError().text();
                return false;
            }

            // 获取最后插入的ID
            node->id = query.lastInsertId().toInt();

            query.clear();

            // 插入节点路径记录
            query.prepare(part_2nd);
            query.bindValue(":id", node->id);
            query.bindValue(":parent", parent_id);

            if (!query.exec()) {
                qWarning() << "Failed to insert node_path record: " << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed to insert record";
        return false;
    }

    return true;
}

bool TreeModel::removeRow(int row, const QModelIndex& parent)
{
    if (row < 0)
        return false;

    auto parent_node = GetNode(parent);
    if (row >= parent_node->children.size())
        return false;

    auto node = parent_node->children.at(row);
    if (!node)
        return false;
    int node_id = node->id;

    beginRemoveRows(parent, row, row);
    if (!node->children.isEmpty()) {
        for (auto child : node->children) {
            child->parent = parent_node;
            parent_node->children.emplace_back(child);
        }
    }
    parent_node->children.removeOne(node);
    endRemoveRows();

    RecalculateAllTotal(node_hash_);

    NodePool::Instance().Recycle(node);
    RemoveRecord(node_id);

    node_hash_.remove(node_id);
    CreatePathForAll(node_hash_, leaf_map_);
    emit SendLeaf(leaf_map_);

    return true;
}

bool TreeModel::RemoveRecord(int id)
{
    QSqlQuery query(db_);
    // query.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("DELETE FROM %1 WHERE id = :id ").arg(tree_info_.node_table);
    const auto part_2nd = QString("UPDATE %1 SET distance = distance - 1 "
                                  "WHERE (descendant IN (SELECT descendant FROM %1 "
                                  "WHERE ancestor = :id AND ancestor != descendant) AND ancestor IN (SELECT ancestor FROM %1 "
                                  "WHERE descendant = :id AND ancestor != descendant)) ")
                              .arg(tree_info_.node_path_table);
    const auto part_3rd = QString("DELETE FROM %1 WHERE descendant = :id OR ancestor = :id").arg(tree_info_.node_path_table);

    if (!DBTransaction([&]() {
            query.prepare(part_1st);
            query.bindValue(":id", id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node record 1st step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(part_2nd);
            query.bindValue(":id", id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node_path record 2nd step: " << query.lastError().text();
                return false;
            }

            query.clear();

            query.prepare(part_3rd);
            query.bindValue(":id", id);
            if (!query.exec()) {
                qWarning() << "Failed to remove node_path record 3rd step: " << query.lastError().text();
                return false;
            }

            return true;
        })) {
        qWarning() << "Failed to remove node";
        return false;
    }

    return true;
}

bool TreeModel::DragRecord(int destination_parent_id, int id)
{
    QSqlQuery query(db_);
    // query.exec("PRAGMA foreign_keys = ON;");

    const auto part_1st = QString("DELETE FROM %1 WHERE (descendant IN (SELECT descendant FROM "
                                  "%1 WHERE ancestor = :id) AND ancestor IN (SELECT ancestor FROM "
                                  "%1 WHERE descendant = :id AND ancestor != descendant)) ")
                              .arg(tree_info_.node_path_table);
    const auto part_2nd = QString("INSERT INTO  %1 (ancestor, descendant, distance) "
                                  "SELECT p.ancestor, s.descendant, p.distance + s.distance + 1 "
                                  "FROM %1 p CROSS JOIN "
                                  "%1 s WHERE p.descendant = :target_parent AND s.ancestor = :id ")
                              .arg(tree_info_.node_path_table);

    if (!DBTransaction([&]() {
            // 第一个查询
            query.prepare(part_1st);
            query.bindValue(":id", id);

            if (!query.exec()) {
                qWarning() << "Failed to drag node_path record 1st step: " << query.lastError().text();
                return false;
            }

            query.clear();

            // 第二个查询
            query.prepare(part_2nd);
            query.bindValue(":id", id);
            query.bindValue(":target_parent", destination_parent_id);

            if (!query.exec()) {
                qWarning() << "Failed to drag node_path record 2nd step: " << query.lastError().text();
                return false;
            }
            return true;
        })) {
        qWarning() << "Failed to drag node";
        return false;
    }

    return true;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return header_.at(section);

    return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    auto flags = QAbstractItemModel::flags(index);
    const int kColumn = index.column();

    switch (kColumn) {
    case Tree::kName:
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        break;
    case Tree::kDescription:
        flags |= Qt::ItemIsEditable;
        break;
    case Tree::kRule:
        flags |= Qt::ItemIsEditable;
        break;
    case Tree::kPlaceholder:
        flags |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }

    return flags;
}

Qt::DropActions TreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList TreeModel::mimeTypes() const
{
    return QStringList() << "node/id";
}

QMimeData* TreeModel::mimeData(const QModelIndexList& indexes) const
{
    auto mime_data = new QMimeData();
    if (indexes.isEmpty())
        return mime_data;

    const auto& first_index = indexes.first();

    if (first_index.isValid()) {
        int id = first_index.sibling(first_index.row(), Tree::kID).data().toInt();
        mime_data->setData("node/id", QByteArray::number(id));
    }

    return mime_data;
}

bool TreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int, int, const QModelIndex&) const
{
    return action != Qt::IgnoreAction && data && data->hasFormat("node/id");
}

bool TreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    auto destination_parent = GetNode(parent);
    if (!destination_parent->placeholder)
        return false;

    int id {};

    auto mime_data = data->data("node/id");
    if (!mime_data.isEmpty()) {
        id = QVariant(mime_data).toInt();
    }

    int begin_row = row == -1 ? destination_parent->children.size() : row;

    auto node = GetNode(id);
    if (node == root_)
        return false;

    if (node && node->parent != destination_parent && !IsDescendant(destination_parent, node)) {
        int source_row = node->parent->children.indexOf(node);
        auto source_index = createIndex(node->parent->children.indexOf(node), 0, node);

        if (beginMoveRows(source_index.parent(), source_row, source_row, parent, begin_row)) {
            node->parent->children.removeAt(source_row);
            CalculatePlaceholderTotal(node, -node->total);

            destination_parent->children.insert(begin_row, node);
            node->parent = destination_parent;
            CalculatePlaceholderTotal(node, node->total);

            endMoveRows();

            DragRecord(destination_parent->id, id);
        }
    }

    CreatePathForAll(node_hash_, leaf_map_);
    emit SendLeaf(leaf_map_);
    return true;
}

bool TreeModel::DBTransaction(auto Function)
{
    if (db_.transaction() && Function() && db_.commit()) {
        return true;
    } else {
        db_.rollback();
        qWarning() << "Transaction failed";
        return false;
    }
}
