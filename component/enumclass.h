#ifndef ENUMCLASS_H
#define ENUMCLASS_H

// Enum class defining sections
enum class Section { kFinance, kSales, kTask, kStakeholder, kProduct, kPurchase };

// Enum class defining transaction columns
enum class PartTableColumn { kID, kDateTime, kCode, kRatio, kDescription, kTransport, kDocument, kState, kRelatedNode, kDebit, kCredit, kRemainder };

// Enum class defining search transaction columns
enum class TableColumn {
    kID,
    kDateTime,
    kCode,
    kLhsNode,
    kLhsRatio,
    kLhsDebit,
    kLhsCredit,
    kDescription,
    kTransport,
    kDocument,
    kState,
    kRhsCredit,
    kRhsDebit,
    kRhsRatio,
    kRhsNode
};

// Enum class defining node columns
enum class TreeColumn {
    kName,
    kID,
    kCode,
    kFirst,
    kSecond,
    kThird,
    kFourth,
    kFifth,
    kSixth,
    kSeventh,
    kDateTime,
    kDescription,
    kNote,
    kNodeRule,
    kBranch,
    kUnit,
    kInitialTotal,
    kFinalTotal,
    kPlaceholder
};

// Enum class defining check options
enum class Check { kAll, kNone, kReverse };

#endif // ENUMCLASS_H
