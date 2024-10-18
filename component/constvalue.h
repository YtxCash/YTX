#ifndef CONSTVALUE_H
#define CONSTVALUE_H

// Constants for values
static constexpr double DMAX = 1'000'000'000.00;
static constexpr double DMIN = -1'000'000'000.00;
static constexpr double DZERO = 0.0;
static constexpr int HUNDRED = 100;
static constexpr int IMAX = 1'000'000'000;
static constexpr int IMIN = -1'000'000'000;
static constexpr int ROW_HEIGHT = 24;
static constexpr int THIRTY_ONE = 31;
static constexpr int IZERO = 0;

// Constants for unit
static constexpr int UNIT_CASH = 0;
static constexpr int UNIT_MONTHLY = 1;
static constexpr int UNIT_PENDING = 2;
static constexpr int UNIT_EMPLOYEE = 0;
static constexpr int UNIT_CUSTOMER = 1;
static constexpr int UNIT_VENDOR = 2;
static constexpr int UNIT_PRODUCT = 3;
static constexpr int UNIT_POSITION = 1;

static constexpr int RULE_CASH = 0;
static constexpr int RULE_MONTHLY = 1;

// Constants for datetime
static constexpr char DATE_FORMAT[] = "date_format";
static constexpr char DATE_FST[] = "yyyy-MM-dd";
static constexpr char DATE_TIME[] = "date_time";
static constexpr char DATE_TIME_FST[] = "yyyy-MM-dd hh:mm";
static constexpr char DATE_TIME_SND[] = "yyyy-MM-dd";
static constexpr char DD[] = "dd";

static constexpr char FULL_WIDTH_PERIOD[] = u8"。";
static constexpr char HALF_WIDTH_PERIOD[] = u8".";

// Constants for separators
static constexpr char COLON[] = ":";
static constexpr char DASH[] = "-";
static constexpr char SEPARATOR[] = "separator";
static constexpr char SLASH[] = "/";

// Constants for operators
static constexpr char MINUS[] = "-";
static constexpr char PLUS[] = "+";

// Constants for table column check state, kState
static constexpr char CHECK[] = "check";

// Constants for files' suffix
static constexpr char SFX_INI[] = ".ini";
static constexpr char SFX_LOCK[] = ".lock";
static constexpr char SFX_QM[] = ".qm";
static constexpr char SFX_QSS[] = ".qss";
static constexpr char SFX_YTX[] = ".ytx";
static constexpr char SFX_PERCENT[] = "%";

// Constants for app's language
static constexpr char EN_US[] = "en_US";
static constexpr char LANGUAGE[] = "language";
static constexpr char ZH_CN[] = "zh_CN";

// Constants for tree and table's column
static constexpr char BRANCH[] = "branch";
static constexpr char CODE[] = "code";
static constexpr char COLOR[] = "color";
static constexpr char COMMISSION[] = "commission";
static constexpr char DEADLINE[] = "deadline";
static constexpr char DESCRIPTION[] = "description";
static constexpr char DISCOUNT[] = "discount";
static constexpr char DOCUMENT[] = "document";
static constexpr char EMPLOYEE[] = "employee";
static constexpr char FINAL_TOTAL[] = "final_total";
static constexpr char FIRST[] = "first";
static constexpr char INITIAL_TOTAL[] = "initial_total";
static constexpr char INITIAL_SUBTOTAL[] = "initial_subtotal";
static constexpr char AMOUNT[] = "amount";
static constexpr char NAME[] = "name";
static constexpr char NODE_ID[] = "node/id";
static constexpr char RULE[] = "rule";
static constexpr char NOTE[] = "note";
static constexpr char PAYMENT_PERIOD[] = "payment_period";
static constexpr char LOCKED[] = "locked";
static constexpr char SECOND[] = "second";
static constexpr char STATE[] = "state";
static constexpr char SETTLED[] = "settled";
static constexpr char TAX_RATE[] = "tax_rate";
static constexpr char UNIT[] = "unit";
static constexpr char LHS_RATIO[] = "lhs_ratio";
static constexpr char UNIT_PRICE[] = "unit_price";
static constexpr char DISCOUNT_PRICE[] = "discount_price";
static constexpr char UNIT_COST[] = "unit_cost";
static constexpr char INSIDE_PRODUCT[] = "inside_product";
static constexpr char OUTSIDE_PRODUCT[] = "outside_product";

// Constants for app's state
static constexpr char HEADER_STATE[] = "header_state";
static constexpr char INTERFACE[] = "interface";
static constexpr char MAINWINDOW_GEOMETRY[] = "mainwindow_geometry";
static constexpr char MAINWINDOW_STATE[] = "mainwindow_state";
static constexpr char SOLARIZED_DARK[] = "Solarized Dark";
static constexpr char SPLITTER_STATE[] = "splitter_state";
static constexpr char THEME[] = "theme";
static constexpr char VIEW[] = "view";
static constexpr char WINDOW[] = "window";

// Constants for others
static constexpr char QSQLITE[] = "QSQLITE";
static constexpr char RECENT_FILE[] = "recent/file";
static constexpr char SEMICOLON[] = ";";
static constexpr char START_SECTION[] = "start/section";
static constexpr char YTX[] = "ytx";

// Constants for sections
static constexpr char Finance[] = "Finance";
static constexpr char FINANCE[] = "finance";
static constexpr char FINANCE_PATH[] = "finance_path";
static constexpr char FINANCE_TRANSACTION[] = "finance_transaction";
static constexpr char PARTY[] = "party";
static constexpr char Product[] = "Product";
static constexpr char PRODUCT[] = "product";
static constexpr char PRODUCT_PATH[] = "product_path";
static constexpr char PRODUCT_TRANSACTION[] = "product_transaction";
static constexpr char Purchase[] = "Purchase";
static constexpr char PURCHASE[] = "purchase";
static constexpr char PURCHASE_PATH[] = "purchase_path";
static constexpr char PURCHASE_TRANSACTION[] = "purchase_transaction";
static constexpr char Sales[] = "Sales";
static constexpr char SALES[] = "sales";
static constexpr char SALES_PATH[] = "sales_path";
static constexpr char SALES_TRANSACTION[] = "sales_transaction";
static constexpr char Stakeholder[] = "Stakeholder";
static constexpr char STAKEHOLDER[] = "stakeholder";
static constexpr char STAKEHOLDER_PATH[] = "stakeholder_path";
static constexpr char STAKEHOLDER_TRANSACTION[] = "stakeholder_transaction";
static constexpr char Task[] = "Task";
static constexpr char TASK[] = "task";
static constexpr char TASK_PATH[] = "task_path";
static constexpr char TASK_TRANSACTION[] = "task_transaction";

#endif // CONSTVALUE_H
