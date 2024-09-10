#ifndef TREEDOUBLESPINR_H
#define TREEDOUBLESPINR_H

// read only

#include "delegate/styleditemdelegate.h"

class TreeDoubleSpinR final : public StyledItemDelegate {
public:
    TreeDoubleSpinR(const int& decimal, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int& decimal_ {};
};

#endif // TREEDOUBLESPINR_H
