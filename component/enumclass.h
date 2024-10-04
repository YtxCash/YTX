#ifndef ENUMCLASS_H
#define ENUMCLASS_H

// Enum class defining sections
enum class Section { kFinance, kSales, kTask, kStakeholder, kProduct, kPurchase };

// Enum class defining trans columns
enum class TableEnum { kID, kDateTime, kCode, kRatio, kDescription, kDocument, kState, kRelatedNode, kDebit, kCredit, kSubtotal };

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
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

enum class TableEnumStakeholder { kID, kDateTime, kCode, kLhsRatio, kDescription, kDocument, kState, kRhsNode };

enum class TableEnumOrder { kID, kLhsNode, kLhsRatio, kCode, kDescription, kFirst, kSecond, kRhsNode, kRhsRatio, kDiscount, kInitialSubtotal, kNodeID };

// Enum class defining node columns
enum class TreeEnum {
    kName,
    kID,
    kCode,
    kDescription,
    kNote,
    kRule,
    kBranch,
    kUnit,
    kFirst,
    kSecond,
    kThird,
    kFourth,
    kFifth,
    kSixth,
    kSeventh,
    kDateTime,
    kInitialTotal,
    kFinalTotal,
    kPlaceholder
};

enum class TreeEnumFinanceTask { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kInitialTotal, kFinalTotal, kPlaceholder };

enum class TreeEnumProduct { kName, kID, kCode, kDescription, kNote, kRule, kBranch, kUnit, kCommission, kUnitPrice, kInitialTotal, kFinalTotal, kPlaceholder };

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
    kDiscount,
    kLocked,
    kInitialTotal,
    kFinalTotal
};

// Enum class defining check options
enum class Check { kAll, kNone, kReverse };

#endif // ENUMCLASS_H
