#ifndef TREEDOUBLESPINDYNAMICUNITR_H
#define TREEDOUBLESPINDYNAMICUNITR_H

// read only

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class TreeDoubleSpinDynamicUnitR final : public StyledItemDelegate {
public:
    TreeDoubleSpinDynamicUnitR(const int& decimal, CStringHash& unit_symbol_hash, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QModelIndex& index) const;

private:
    const int& decimal_ {};
    CStringHash& unit_symbol_hash_;
};

#endif // TREEDOUBLESPINDYNAMICUNITR_H
