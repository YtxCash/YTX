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
inline constexpr long long kBatchSize = 50;
inline constexpr double kDoubleMax = 1'000'000'000.00;
inline constexpr double kDoubleMin = -1'000'000'000.00;
inline constexpr int kHundred = 100;
inline constexpr int kIntMax = 1'000'000'000;
inline constexpr int kRowHeight = 24;
inline constexpr int kThreeThousand = 3000;

// Constants for rule
inline constexpr bool kRuleIM = 0;
inline constexpr bool kRuleMS = 1;

// Constants for datetime
inline constexpr char kDateFormat[] = "date_format";
inline constexpr char kDateTime[] = "date_time";
inline constexpr char kDateTimeFST[] = "yyyy-MM-dd hh:mm";
inline constexpr char kDateFST[] = "yyyy-MM-dd";
inline constexpr char kDD[] = "dd";

inline constexpr char kFullWidthPeriod[] = u8"。";
inline constexpr char kHalfWidthPeriod[] = u8".";

// Constants for separators
inline constexpr char kColon[] = ":";
inline constexpr char kDash[] = "-";
inline constexpr char kSeparator[] = "separator";
inline constexpr char kSlash[] = "/";

// Constants for operators
inline constexpr char kMinux[] = "-";
inline constexpr char kPlus[] = "+";

// Constants for table column check state, kState
inline constexpr char kCheck[] = "check";

// Constants for files' suffix
inline constexpr char kSuffixINI[] = ".ini";
inline constexpr char kSuffixLOCK[] = ".lock";
inline constexpr char kSuffixQM[] = ".qm";
inline constexpr char kSuffixQSS[] = ".qss";
inline constexpr char kSuffixYTX[] = ".ytx";
inline constexpr char kSuffixPERCENT[] = "%";

// Constants for app's language
inline constexpr char kEnUS[] = "en_US";
inline constexpr char kLanguage[] = "language";
inline constexpr char kZhCN[] = "zh_CN";

// Constants for tree and table's column
inline constexpr char kType[] = "type";
inline constexpr char kCode[] = "code";
inline constexpr char kColor[] = "color";
inline constexpr char kCommission[] = "commission";
inline constexpr char kDeadline[] = "deadline";
inline constexpr char kDescription[] = "description";
inline constexpr char kDiscount[] = "discount";
inline constexpr char kDocument[] = "document";
inline constexpr char kEmployee[] = "employee";
inline constexpr char kFirst[] = "first";
inline constexpr char kAmount[] = "amount";
inline constexpr char kName[] = "name";
inline constexpr char kNodeID[] = "node/id";
inline constexpr char kRule[] = "rule";
inline constexpr char kNote[] = "note";
inline constexpr char kParty[] = "party";
inline constexpr char kPaymentPeriod[] = "payment_period";
inline constexpr char kFinished[] = "finished";
inline constexpr char kSecond[] = "second";
inline constexpr char kState[] = "state";
inline constexpr char kSettled[] = "settled";
inline constexpr char kTaxRate[] = "tax_rate";
inline constexpr char kUnit[] = "unit";
inline constexpr char kUnitPrice[] = "unit_price";
inline constexpr char kDiscountPrice[] = "discount_price";
inline constexpr char kUnitCost[] = "unit_cost";
inline constexpr char kInsideProduct[] = "inside_product";
inline constexpr char kOutsideProduct[] = "outside_product";
inline constexpr char kSupportID[] = "support_id";

// Constants for app's state
inline constexpr char kHeaderState[] = "header_state";
inline constexpr char kInterface[] = "interface";
inline constexpr char kMainwindowGeometry[] = "mainwindow_geometry";
inline constexpr char kMainwindowState[] = "mainwindow_state";
inline constexpr char kSolarizedDark[] = "Solarized Dark";
inline constexpr char kSplitterState[] = "splitter_state";
inline constexpr char kTheme[] = "theme";
inline constexpr char kView[] = "view";
inline constexpr char kWindow[] = "window";

// Constants for others
inline constexpr char kQSQLITE[] = "QSQLITE";
inline constexpr char kRecentFile[] = "recent/file";
inline constexpr char kSemicolon[] = ";";
inline constexpr char kStartSection[] = "start/section";
inline constexpr char kYTX[] = "ytx";

// Constants for sections
inline constexpr char kFINANCE[] = "FINANCE";
inline constexpr char kFinance[] = "finance";
inline constexpr char kFinancePath[] = "finance_path";
inline constexpr char kFinanceTransaction[] = "finance_transaction";
inline constexpr char kPRODUCT[] = "PRODUCT";
inline constexpr char kProduct[] = "product";
inline constexpr char kProductPath[] = "product_path";
inline constexpr char kProductTransaction[] = "product_transaction";
inline constexpr char kPURCHASE[] = "PURCHASE";
inline constexpr char kPurchase[] = "purchase";
inline constexpr char kPurchasePath[] = "purchase_path";
inline constexpr char kPurchaseTransaction[] = "purchase_transaction";
inline constexpr char kSALES[] = "SALES";
inline constexpr char kSales[] = "sales";
inline constexpr char kSalesPath[] = "sales_path";
inline constexpr char kSalesTransaction[] = "sales_transaction";
inline constexpr char kSTAKEHOLDER[] = "STAKEHOLDER";
inline constexpr char kStakeholder[] = "stakeholder";
inline constexpr char kStakeholderPath[] = "stakeholder_path";
inline constexpr char kStakeholderTransaction[] = "stakeholder_transaction";
inline constexpr char kTASK[] = "TASK";
inline constexpr char kTask[] = "task";
inline constexpr char kTaskPath[] = "task_path";
inline constexpr char kTaskTransaction[] = "task_transaction";

#endif // CONSTVALUE_H
