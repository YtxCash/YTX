#include "mainwindowsqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/sqlconnection.h"

MainwindowSqlite::MainwindowSqlite(Section section)
    : db_ { SqlConnection::Instance().Allocate(section) }
{
}

void MainwindowSqlite::QuerySectionRule(SectionRule& section_rule, Section section)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = R"(
    SELECT static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, hide_time, base_unit, document_dir, value_decimal, ratio_decimal
    FROM section_rule
    WHERE id = :section
)";

    query.prepare(part);
    query.bindValue(":section", std::to_underlying(section) + 1);
    if (!query.exec()) {
        qWarning() << "Failed to query section rule: " << query.lastError().text();
        return;
    }

    while (query.next()) {
        section_rule.static_label = query.value("static_label").toString();
        section_rule.static_node = query.value("static_node").toInt();
        section_rule.dynamic_label = query.value("dynamic_label").toString();
        section_rule.dynamic_node_lhs = query.value("dynamic_node_lhs").toInt();
        section_rule.operation = query.value("operation").toString();
        section_rule.dynamic_node_rhs = query.value("dynamic_node_rhs").toInt();
        section_rule.hide_time = query.value("hide_time").toBool();
        section_rule.base_unit = query.value("base_unit").toInt();
        section_rule.document_dir = query.value("document_dir").toString();
        section_rule.value_decimal = query.value("value_decimal").toInt();
        section_rule.ratio_decimal = query.value("ratio_decimal").toInt();
    }
}

void MainwindowSqlite::UpdateSectionRule(CSectionRule& section_rule, Section section)
{
    auto part = R"(
    UPDATE section_rule
    SET static_label = :static_label, static_node = :static_node, dynamic_label = :dynamic_label, dynamic_node_lhs = :dynamic_node_lhs,
        operation = :operation, dynamic_node_rhs = :dynamic_node_rhs, hide_time = :hide_time, base_unit = :base_unit, document_dir = :document_dir,
        value_decimal = :value_decimal, ratio_decimal = :ratio_decimal
    WHERE id = :section
)";

    QSqlQuery query(*db_);

    query.prepare(part);
    query.bindValue(":section", std::to_underlying(section) + 1);
    query.bindValue(":static_label", section_rule.static_label);
    query.bindValue(":static_node", section_rule.static_node);
    query.bindValue(":dynamic_label", section_rule.dynamic_label);
    query.bindValue(":dynamic_node_lhs", section_rule.dynamic_node_lhs);
    query.bindValue(":operation", section_rule.operation);
    query.bindValue(":dynamic_node_rhs", section_rule.dynamic_node_rhs);
    query.bindValue(":hide_time", section_rule.hide_time);
    query.bindValue(":base_unit", section_rule.base_unit);
    query.bindValue(":document_dir", section_rule.document_dir);
    query.bindValue(":value_decimal", section_rule.value_decimal);
    query.bindValue(":ratio_decimal", section_rule.ratio_decimal);

    if (!query.exec()) {
        qWarning() << "Failed to update section rule: " << query.lastError().text();
        return;
    }
}

void MainwindowSqlite::NewFile(CString& file_path)
{
    QSqlDatabase db { QSqlDatabase::addDatabase(QSQLITE) };
    db.setDatabaseName(file_path);
    if (!db.open())
        return;

    QString finance = NodeFinanceTask(FINANCE);
    QString finance_path = Path(FINANCE_PATH);
    QString finance_transaction = TransactionFinance(FINANCE_TRANSACTION);

    QString product = NodeProduct(PRODUCT);
    QString product_path = Path(PRODUCT_PATH);
    QString product_transaction = TransactionProduct(PRODUCT_TRANSACTION);

    QString task = NodeFinanceTask(TASK);
    QString task_path = Path(TASK_PATH);
    QString task_transaction = TransactionTask(TASK_TRANSACTION);

    QString stakeholder = NodeStakeholder(STAKEHOLDER);
    QString stakeholder_path = Path(STAKEHOLDER_PATH);
    QString stakeholder_transaction = TransactionStakeholder(STAKEHOLDER_TRANSACTION);

    QString purchase = NodeOrder(PURCHASE);
    QString purchase_path = Path(PURCHASE_PATH);
    QString purchase_transaction = TransactionOrder(PURCHASE_TRANSACTION);

    QString sales = NodeOrder(SALES);
    QString sales_path = Path(SALES_PATH);
    QString sales_transaction = TransactionOrder(SALES_TRANSACTION);

    QString section_rule = R"(
    CREATE TABLE IF NOT EXISTS section_rule (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        static_label        TEXT,
        static_node         INTEGER,
        dynamic_label       TEXT,
        dynamic_node_lhs    INTEGER,
        operation           TEXT,
        dynamic_node_rhs    INTEGER,
        hide_time           BOOLEAN    DEFAULT 1,
        base_unit           INTEGER,
        document_dir        TEXT,
        value_decimal       INTEGER    DEFAULT 2,
        ratio_decimal       INTEGER    DEFAULT 4
    );
)";

    QString section_rule_row = "INSERT INTO section_rule (static_node) VALUES (0);";

    QSqlQuery query {};
    if (db.transaction()) {
        // Execute each table creation query
        if (query.exec(finance) && query.exec(finance_path) && query.exec(finance_transaction) && query.exec(product) && query.exec(product_path)
            && query.exec(product_transaction) && query.exec(stakeholder) && query.exec(stakeholder_path) && query.exec(stakeholder_transaction)
            && query.exec(task) && query.exec(task_path) && query.exec(task_transaction) && query.exec(purchase) && query.exec(purchase_path)
            && query.exec(purchase_transaction) && query.exec(sales) && query.exec(sales_path) && query.exec(sales_transaction) && query.exec(section_rule)) {
            // Commit the transaction if all queries are successful
            if (db.commit()) {
                for (int i = 0; i != 6; ++i) {
                    query.exec(section_rule_row);
                }
            } else {
                // Handle commit failure
                qDebug() << "Error committing transaction:" << db.lastError().text();
                // Rollback the transaction in case of failure
                db.rollback();
            }
        } else {
            // Handle query execution failure
            qDebug() << "Error creating tables:" << query.lastError().text();
            // Rollback the transaction in case of failure
            db.rollback();
        }
    } else {
        // Handle transaction start failure
        qDebug() << "Error starting transaction:" << db.lastError().text();
    }

    db.close();
}

QString MainwindowSqlite::NodeFinanceTask(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        node_rule        BOOLEAN    DEFAULT 0,
        branch           BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        initial_total    NUMERIC,
        final_total      NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::NodeStakeholder(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        node_rule         BOOLEAN    DEFAULT 0,
        branch            BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        deadline          INTEGER,
        employee          INTEGER,
        payment_period    INTEGER,
        tax_rate          NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::NodeProduct(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        node_rule        BOOLEAN    DEFAULT 0,
        branch           BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        commission       NUMERIC,
        unit_price       NUMERIC,
        initial_total    NUMERIC,
        final_total      NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::NodeOrder(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        node_rule         BOOLEAN    DEFAULT 0,
        branch            BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        party             INTEGER,
        employee          INTEGER,
        date_time         DATE,
        first             INTEGER,
        second            NUMERIC,
        discount          NUMERIC,
        posted            BOOLEAN    DEFAULT 0,
        initial_total     NUMERIC,
        final_total       NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::Path(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        ancestor      INTEGER    CHECK (ancestor   >= 1),
        descendant    INTEGER    CHECK (descendant >= 1),
        distance      INTEGER    CHECK (distance   >= 0)
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::TransactionFinance(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC    DEFAULT 1.0    CHECK (lhs_ratio   > 0),
        lhs_debit      NUMERIC                   CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                   CHECK (lhs_credit >= 0),
        description    TEXT,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                   CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                   CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC    DEFAULT 1.0    CHECK (rhs_ratio   > 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::TransactionOrder(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC    DEFAULT 1.0    CHECK (lhs_ratio   > 0),
        lhs_debit      NUMERIC                   CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                   CHECK (lhs_credit >= 0),
        description    TEXT,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                   CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                   CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC    DEFAULT 1.0    CHECK (rhs_ratio   > 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::TransactionStakeholder(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        outside        INTEGER,
        unit_price     NUMERIC,
        description    TEXT,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        commission     NUMERIC,
        inside         INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::TransactionTask(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC,
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}

QString MainwindowSqlite::TransactionProduct(CString& table_name)
{
    return QString(R"(
    CREATE TABLE IF NOT EXISTS %1 (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_ratio      NUMERIC,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_ratio      NUMERIC,
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
)")
        .arg(table_name);
}
