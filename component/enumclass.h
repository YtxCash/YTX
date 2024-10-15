#ifndef ENUMCLASS_H
#define ENUMCLASS_H

// Enum class defining sections
enum class Section { kFinance, kProduct, kTask, kStakeholder, kSales, kPurchase };

// Enum class defining trans columns
enum class TableEnum { kID, kDateTime, kCode, kLhsRatio, kDescription, kDocument, kState, kRhsNode, kDebit, kCredit, kSubtotal };

// Enum class defining search trans columns
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
    kNodeID,
    kDiscountPrice,
    kSettled,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

enum class TableEnumStakeholder { kID, kDateTime, kCode, kOutsideProduct, kDescription, kUnitPrice, kDocument, kState, kInsideProduct };

enum class TableEnumOrder {
    kID,
    kInsideProduct,
    kUnitPrice,
    kCode,
    kDescription,
    kColor,
    kNodeID,
    kFirst,
    kSecond,
    kAmount,
    kDiscountPrice,
    kDiscount,
    kSettled,
    kOutsideProduct
};

// Enum class defining node columns
enum class TreeEnumCommon {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kBranch,
    kUnit,
};

enum class TreeEnum { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kInitialTotal, kFinalTotal, kPlaceholder };

enum class TreeEnumTask { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kColor, kUnitCost, kQuantity, kAmount, kPlaceholder };

enum class TreeEnumProduct { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kColor, kUnitPrice, kCommission, kQuantity, kAmount, kPlaceholder };

enum class TreeEnumStakeholder { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kDeadline, kEmployee, kPaymentPeriod, kTaxRate, kPlaceholder };

enum class TreeEnumOrder {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kBranch,
    kUnit,
    kParty,
    kEmployee,
    kDateTime,
    kFirst,
    kSecond,
    kLocked,
    kAmount,
    kDiscount,
    kSettled
};

// Enum class defining check options
enum class Check { kAll, kNone, kReverse };

#endif // ENUMCLASS_H
