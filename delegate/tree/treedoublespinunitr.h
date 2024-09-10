#ifndef TREEDOUBLESPINUNITR_H
#define TREEDOUBLESPINUNITR_H

// read only

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class TreeDoubleSpinUnitR final : public StyledItemDelegate {
public:
    TreeDoubleSpinUnitR(const int& decimal, const int& unit, CStringHash& unit_symbol_hash, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QModelIndex& index) const;

private:
    const int& decimal_ {};
    const int& unit_ {};
    CStringHash& unit_symbol_hash_;
};

#endif // TREEDOUBLESPINUNITR_H
