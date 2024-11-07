#include "mainwindowsqlite.h"

#include <QSqlError>
#include <QSqlQuery>

#include "component/constvalue.h"
#include "global/sqlconnection.h"

MainwindowSqlite::MainwindowSqlite(Section section)
    : db_ { SqlConnection::Instance().Allocate(section) }
{
}

void MainwindowSqlite::QuerySettings(Settings& settings, Section section)
{
    QSqlQuery query(*db_);
    query.setForwardOnly(true);

    auto part = R"(
    SELECT static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, default_unit, document_dir, amount_decimal, common_decimal
    FROM settings
    WHERE id = :section
)";

    query.prepare(part);
    query.bindValue(":section", std::to_underlying(section) + 1);
    if (!query.exec()) {
        qWarning() << "Failed to query section settings: " << query.lastError().text();
        return;
    }

    while (query.next()) {
        settings.static_label = query.value("static_label").toString();
        settings.static_node = query.value("static_node").toInt();
        settings.dynamic_label = query.value("dynamic_label").toString();
        settings.dynamic_node_lhs = query.value("dynamic_node_lhs").toInt();
        settings.operation = query.value("operation").toString();
        settings.dynamic_node_rhs = query.value("dynamic_node_rhs").toInt();
        settings.default_unit = query.value("default_unit").toInt();
        settings.document_dir = query.value("document_dir").toString();
        settings.amount_decimal = query.value("amount_decimal").toInt();
        settings.common_decimal = query.value("common_decimal").toInt();
    }
}

void MainwindowSqlite::UpdateSettings(CSettings& settings, Section section)
{
    auto part = R"(
    UPDATE settings SET
        static_label = :static_label, static_node = :static_node, dynamic_label = :dynamic_label, dynamic_node_lhs = :dynamic_node_lhs,
        operation = :operation, dynamic_node_rhs = :dynamic_node_rhs, default_unit = :default_unit, document_dir = :document_dir,
        amount_decimal = :amount_decimal, common_decimal = :common_decimal
    WHERE id = :section
)";

    QSqlQuery query(*db_);

    query.prepare(part);
    query.bindValue(":section", std::to_underlying(section) + 1);
    query.bindValue(":static_label", settings.static_label);
    query.bindValue(":static_node", settings.static_node);
    query.bindValue(":dynamic_label", settings.dynamic_label);
    query.bindValue(":dynamic_node_lhs", settings.dynamic_node_lhs);
    query.bindValue(":operation", settings.operation);
    query.bindValue(":dynamic_node_rhs", settings.dynamic_node_rhs);
    query.bindValue(":default_unit", settings.default_unit);
    query.bindValue(":document_dir", settings.document_dir);
    query.bindValue(":amount_decimal", settings.amount_decimal);
    query.bindValue(":common_decimal", settings.common_decimal);

    if (!query.exec()) {
        qWarning() << "Failed to update section settings: " << query.lastError().text();
        return;
    }
}

void MainwindowSqlite::NewFile(CString& file_path)
{
    QSqlDatabase db { QSqlDatabase::addDatabase(QSQLITE) };
    db.setDatabaseName(file_path);
    if (!db.open())
        return;

    QString finance = NodeFinance();
    QString finance_path = Path(FINANCE_PATH);
    QString finance_transaction = TransactionFinance();

    QString product = NodeProduct();
    QString product_path = Path(PRODUCT_PATH);
    QString product_transaction = TransactionProduct();

    QString task = NodeTask();
    QString task_path = Path(TASK_PATH);
    QString task_transaction = TransactionTask();

    QString stakeholder = NodeStakeholder();
    QString stakeholder_path = Path(STAKEHOLDER_PATH);
    QString stakeholder_transaction = TransactionStakeholder();

    QString purchase = NodePurchase();
    QString purchase_path = Path(PURCHASE_PATH);
    QString purchase_transaction = TransactionPurchase();

    QString sales = NodeSales();
    QString sales_path = Path(SALES_PATH);
    QString sales_transaction = TransactionSales();

    QString settings = QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS settings (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        static_label        TEXT,
        static_node         INTEGER,
        dynamic_label       TEXT,
        dynamic_node_lhs    INTEGER,
        operation           TEXT,
        dynamic_node_rhs    INTEGER,
        default_unit        INTEGER,
        document_dir        TEXT,
        amount_decimal      INTEGER    DEFAULT 2,
        common_decimal      INTEGER    DEFAULT 2
    );
    )");

    QString settings_row = "INSERT INTO settings (static_node) VALUES (0);";

    QSqlQuery query {};
    if (db.transaction()) {
        // Execute each table creation query
        if (query.exec(finance) && query.exec(finance_path) && query.exec(finance_transaction) && query.exec(product) && query.exec(product_path)
            && query.exec(product_transaction) && query.exec(stakeholder) && query.exec(stakeholder_path) && query.exec(stakeholder_transaction)
            && query.exec(task) && query.exec(task_path) && query.exec(task_transaction) && query.exec(purchase) && query.exec(purchase_path)
            && query.exec(purchase_transaction) && query.exec(sales) && query.exec(sales_path) && query.exec(sales_transaction) && query.exec(settings)) {
            // Commit the transaction if all queries are successful
            if (db.commit()) {
                for (int i = 0; i != 6; ++i) {
                    query.exec(settings_row);
                }
            } else {
                // Handle commit failure
                qDebug() << "Error committing transaction" << db.lastError().text();
                // Rollback the transaction in case of failure
                db.rollback();
            }
        } else {
            // Handle query execution failure
            qDebug() << "Error creating tables" << query.lastError().text();
            // Rollback the transaction in case of failure
            db.rollback();
        }
    } else {
        // Handle transaction start failure
        qDebug() << "Error starting transaction" << db.lastError().text();
    }

    db.close();
}

QString MainwindowSqlite::NodeFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        rule             BOOLEAN    DEFAULT 0,
        branch           BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        initial_total    NUMERIC,
        final_total      NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::NodeStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        rule              BOOLEAN    DEFAULT 0,
        branch            BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        deadline          TEXT,
        employee          INTEGER,
        payment_period    INTEGER,
        tax_rate          NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::NodeProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        rule             BOOLEAN    DEFAULT 0,
        branch           BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        color            TEXT,
        commission       NUMERIC,
        unit_price       NUMERIC,
        quantity         NUMERIC,
        amount           NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
)");
}

QString MainwindowSqlite::NodeTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task (
        id               INTEGER PRIMARY KEY AUTOINCREMENT,
        name             TEXT,
        code             TEXT,
        description      TEXT,
        note             TEXT,
        rule             BOOLEAN    DEFAULT 0,
        branch           BOOLEAN    DEFAULT 0,
        unit             INTEGER,
        finished         BOOLEAN    DEFAULT 0,
        date_time        TEXT,
        color            TEXT,
        unit_cost        NUMERIC,
        quantity         NUMERIC,
        amount           NUMERIC,
        removed          BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::NodeSales()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS sales (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        rule              BOOLEAN    DEFAULT 0,
        branch            BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        party             INTEGER,
        employee          INTEGER,
        date_time         TEXT,
        first             NUMERIC,
        second            NUMERIC,
        discount          NUMERIC,
        finished          BOOLEAN    DEFAULT 0,
        amount            NUMERIC,
        settled           NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::NodePurchase()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS purchase (
        id                INTEGER PRIMARY KEY AUTOINCREMENT,
        name              TEXT,
        code              TEXT,
        description       TEXT,
        note              TEXT,
        rule              BOOLEAN    DEFAULT 0,
        branch            BOOLEAN    DEFAULT 0,
        unit              INTEGER,
        party             INTEGER,
        employee          INTEGER,
        date_time         TEXT,
        first             NUMERIC,
        second            NUMERIC,
        discount          NUMERIC,
        finished          BOOLEAN    DEFAULT 0,
        amount            NUMERIC,
        settled           NUMERIC,
        removed           BOOLEAN    DEFAULT 0
    );
    )");
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

QString MainwindowSqlite::TransactionFinance()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS finance_transaction (
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
    )");
}

QString MainwindowSqlite::TransactionSales()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS sales_transaction (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        code                TEXT,
        inside_product      INTEGER,
        first               NUMERIC,
        second              NUMERIC,
        description         TEXT,
        unit_price          NUMERIC,
        node_id             INTEGER,
        discount_price      NUMERIC,
        amount              NUMERIC,
        discount            NUMERIC,
        settled             NUMERIC,
        outside_product     INTEGER,
        removed             BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::TransactionPurchase()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS purchase_transaction (
        id                  INTEGER PRIMARY KEY AUTOINCREMENT,
        code                TEXT,
        inside_product      INTEGER,
        first               NUMERIC,
        second              NUMERIC,
        description         TEXT,
        unit_price          NUMERIC,
        node_id             INTEGER,
        discount_price      NUMERIC,
        amount              NUMERIC,
        discount            NUMERIC,
        settled             NUMERIC,
        outside_product     INTEGER,
        removed             BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::TransactionStakeholder()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS stakeholder_transaction (
        id                 INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time          DATE,
        code               TEXT,
        outside_product    INTEGER,
        description        TEXT,
        unit_price         NUMERIC,
        node_id            INTEGER,
        document           TEXT,
        state              BOOLEAN    DEFAULT 0,
        inside_product     INTEGER,
        removed            BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::TransactionTask()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS task_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        unit_cost      NUMERIC,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
    )");
}

QString MainwindowSqlite::TransactionProduct()
{
    return QStringLiteral(R"(
    CREATE TABLE IF NOT EXISTS product_transaction (
        id             INTEGER PRIMARY KEY AUTOINCREMENT,
        date_time      DATE,
        code           TEXT,
        lhs_node       INTEGER,
        lhs_debit      NUMERIC                  CHECK (lhs_debit  >= 0),
        lhs_credit     NUMERIC                  CHECK (lhs_credit >= 0),
        description    TEXT,
        unit_cost      NUMERIC,
        document       TEXT,
        state          BOOLEAN    DEFAULT 0,
        rhs_credit     NUMERIC                  CHECK (rhs_credit >= 0),
        rhs_debit      NUMERIC                  CHECK (rhs_debit  >= 0),
        rhs_node       INTEGER,
        removed        BOOLEAN    DEFAULT 0
    );
    )");
}
