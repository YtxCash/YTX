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

#ifndef ENUMCLASS_H
#define ENUMCLASS_H

// Enum

enum UnitOrder { kUnitIM, kUnitMS, kUnitPEND };

enum UnitStakeholder { kUnitCust, kUnitEmp, kUnitVend, kUnitProd };

enum UnitProduct { kPlaceholder, kUnitPos };

enum NodeType { kTypeLeaf, kTypeBranch, kTypeSupport };

// Enum class defining sections
enum class Section { kFinance, kProduct, kTask, kStakeholder, kSales, kPurchase };

// for delegate
enum class Filter { kIncludeSpecific, kExcludeSpecific, kIncludeSpecificWithNone, kIncludeAllWithNone };

// Enum class defining trans columns
enum class TableEnum { kID, kDateTime, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode };

enum class TableEnumFinance { kID, kDateTime, kLhsRatio, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumTask { kID, kDateTime, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumProduct { kID, kDateTime, kUnitCost, kCode, kDescription, kSupportID, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

enum class TableEnumStakeholder { kID, kDateTime, kUnitPrice, kCode, kDescription, kOutsideProduct, kDocument, kState, kInsideProduct };

enum class TableEnumOrder {
    kID,
    kInsideProduct,
    kUnitPrice,
    kCode,
    kDescription,
    kOutsideProduct,
    kColor,
    kLhsNode,
    kFirst,
    kSecond,
    kAmount,
    kDiscountPrice,
    kDiscount,
    kSettled,
};

enum class TableEnumSearch {
    kID,
    kDateTime,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kUnitPrice,
    kSupportID,
    kDiscountPrice,
    kSettled,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

enum class TableEnumSupport {
    kID,
    kDateTime,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kUnitPrice,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

// Enum class defining node columns
enum class TreeEnum { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit };

enum class TreeEnumFinance { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kInitialTotal, kFinalTotal };

enum class TreeEnumTask { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kDateTime, kFinished, kColor, kUnitCost, kQuantity, kAmount };

enum class TreeEnumProduct { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kColor, kUnitPrice, kCommission, kQuantity, kAmount };

enum class TreeEnumStakeholder { kName, kID, kCode, kDescription, kNote, kRule, kType, kUnit, kDeadline, kEmployee, kPaymentTerm, kTaxRate };

enum class TreeEnumOrder {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kType,
    kUnit,
    kParty,
    kEmployee,
    kDateTime,
    kFirst,
    kSecond,
    kFinished,
    kAmount,
    kDiscount,
    kSettled
};

enum class TreeEnumSearch {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kType,
    kUnit,
    kParty,
    kEmployee,
    kDateTime,
    kColor,
    kFirst,
    kSecond,
    kDiscount,
    kFinished,
    kInitialTotal,
    kFinalTotal
};

// Enum class defining check options
enum class Check { kNone, kAll, kReverse };

#endif // ENUMCLASS_H
