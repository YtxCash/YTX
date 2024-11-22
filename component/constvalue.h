/*
 * Copyright (C) 2023 YtxCash
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONSTVALUE_H
#define CONSTVALUE_H

// Constants for values
inline constexpr long long BATCH_SIZE = 50;
inline constexpr double DMAX = 1'000'000'000.00;
inline constexpr double DMIN = -1'000'000'000.00;
inline constexpr int HUNDRED = 100;
inline constexpr int IMAX = 1'000'000'000;
inline constexpr int IMIN = -1'000'000'000;
inline constexpr int ROW_HEIGHT = 24;
inline constexpr int THIRTY_ONE = 31;
inline constexpr int THREE_THOUSAND = 3000;

// Constants for unit
inline constexpr int UNIT_IM = 0;
inline constexpr int UNIT_MS = 1;
inline constexpr int UNIT_PEND = 2;

// Constants for node type
inline constexpr int kTypeLeaf = 0;
inline constexpr int kTypeBranch = 1;
inline constexpr int kTypeSupport = 2;

inline constexpr int UNIT_CUST = 0;
inline constexpr int UNIT_EMP = 1;
inline constexpr int UNIT_VEND = 2;
inline constexpr int UNIT_PROD = 3;

inline constexpr int UNIT_POS = 1;

// Constants for rule
inline constexpr int RULE_IM = 0;
inline constexpr int RULE_MS = 1;

// Constants for datetime
inline constexpr char DATE_FORMAT[] = "date_format";
inline constexpr char DATE_TIME[] = "date_time";
inline constexpr char DATE_TIME_FST[] = "yyyy-MM-dd hh:mm";
inline constexpr char DATE_FST[] = "yyyy-MM-dd";
inline constexpr char DD[] = "dd";

inline constexpr char FULL_WIDTH_PERIOD[] = u8"。";
inline constexpr char HALF_WIDTH_PERIOD[] = u8".";

// Constants for separators
inline constexpr char COLON[] = ":";
inline constexpr char DASH[] = "-";
inline constexpr char SEPARATOR[] = "separator";
inline constexpr char SLASH[] = "/";

// Constants for operators
inline constexpr char MINUS[] = "-";
inline constexpr char PLUS[] = "+";

// Constants for table column check state, kState
inline constexpr char CHECK[] = "check";

// Constants for files' suffix
inline constexpr char SFX_INI[] = ".ini";
inline constexpr char SFX_LOCK[] = ".lock";
inline constexpr char SFX_QM[] = ".qm";
inline constexpr char SFX_QSS[] = ".qss";
inline constexpr char SFX_YTX[] = ".ytx";
inline constexpr char SFX_PERCENT[] = "%";

// Constants for app's language
inline constexpr char EN_US[] = "en_US";
inline constexpr char LANGUAGE[] = "language";
inline constexpr char ZH_CN[] = "zh_CN";

// Constants for tree and table's column
inline constexpr char TYPE[] = "type";
inline constexpr char CODE[] = "code";
inline constexpr char COLOR[] = "color";
inline constexpr char COMMISSION[] = "commission";
inline constexpr char DEADLINE[] = "deadline";
inline constexpr char DESCRIPTION[] = "description";
inline constexpr char DISCOUNT[] = "discount";
inline constexpr char DOCUMENT[] = "document";
inline constexpr char EMPLOYEE[] = "employee";
inline constexpr char FINAL_TOTAL[] = "final_total";
inline constexpr char FIRST[] = "first";
inline constexpr char INITIAL_TOTAL[] = "initial_total";
inline constexpr char INITIAL_SUBTOTAL[] = "initial_subtotal";
inline constexpr char AMOUNT[] = "amount";
inline constexpr char NAME[] = "name";
inline constexpr char NODE_ID[] = "node/id";
inline constexpr char RULE[] = "rule";
inline constexpr char NOTE[] = "note";
inline constexpr char PAYMENT_PERIOD[] = "payment_period";
inline constexpr char FINISHED[] = "finished";
inline constexpr char SECOND[] = "second";
inline constexpr char STATE[] = "state";
inline constexpr char SETTLED[] = "settled";
inline constexpr char TAX_RATE[] = "tax_rate";
inline constexpr char UNIT[] = "unit";
inline constexpr char LHS_RATIO[] = "lhs_ratio";
inline constexpr char UNIT_PRICE[] = "unit_price";
inline constexpr char DISCOUNT_PRICE[] = "discount_price";
inline constexpr char UNIT_COST[] = "unit_cost";
inline constexpr char INSIDE_PRODUCT[] = "inside_product";
inline constexpr char OUTSIDE_PRODUCT[] = "outside_product";
inline constexpr char HELPER_ID[] = "helper_id";

// Constants for app's state
inline constexpr char HEADER_STATE[] = "header_state";
inline constexpr char INTERFACE[] = "interface";
inline constexpr char MAINWINDOW_GEOMETRY[] = "mainwindow_geometry";
inline constexpr char MAINWINDOW_STATE[] = "mainwindow_state";
inline constexpr char SOLARIZED_DARK[] = "Solarized Dark";
inline constexpr char SPLITTER_STATE[] = "splitter_state";
inline constexpr char THEME[] = "theme";
inline constexpr char VIEW[] = "view";
inline constexpr char WINDOW[] = "window";

// Constants for others
inline constexpr char QSQLITE[] = "QSQLITE";
inline constexpr char RECENT_FILE[] = "recent/file";
inline constexpr char SEMICOLON[] = ";";
inline constexpr char START_SECTION[] = "start/section";
inline constexpr char YTX[] = "ytx";

// Constants for sections
inline constexpr char Finance[] = "Finance";
inline constexpr char FINANCE[] = "finance";
inline constexpr char FINANCE_PATH[] = "finance_path";
inline constexpr char FINANCE_TRANSACTION[] = "finance_transaction";
inline constexpr char PARTY[] = "party";
inline constexpr char Product[] = "Product";
inline constexpr char PRODUCT[] = "product";
inline constexpr char PRODUCT_PATH[] = "product_path";
inline constexpr char PRODUCT_TRANSACTION[] = "product_transaction";
inline constexpr char Purchase[] = "Purchase";
inline constexpr char PURCHASE[] = "purchase";
inline constexpr char PURCHASE_PATH[] = "purchase_path";
inline constexpr char PURCHASE_TRANSACTION[] = "purchase_transaction";
inline constexpr char Sales[] = "Sales";
inline constexpr char SALES[] = "sales";
inline constexpr char SALES_PATH[] = "sales_path";
inline constexpr char SALES_TRANSACTION[] = "sales_transaction";
inline constexpr char Stakeholder[] = "Stakeholder";
inline constexpr char STAKEHOLDER[] = "stakeholder";
inline constexpr char STAKEHOLDER_PATH[] = "stakeholder_path";
inline constexpr char STAKEHOLDER_TRANSACTION[] = "stakeholder_transaction";
inline constexpr char Task[] = "Task";
inline constexpr char TASK[] = "task";
inline constexpr char TASK_PATH[] = "task_path";
inline constexpr char TASK_TRANSACTION[] = "task_transaction";

#endif // CONSTVALUE_H
