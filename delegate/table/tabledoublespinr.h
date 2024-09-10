#ifndef TABLEDOUBLESPINR_H
#define TABLEDOUBLESPINR_H

#include "delegate/styleditemdelegate.h"

class TableDoubleSpinR final : public StyledItemDelegate {
public:
    TableDoubleSpinR(const int& decimal, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int& decimal_;
};

#endif // TABLEDOUBLESPINR_H
