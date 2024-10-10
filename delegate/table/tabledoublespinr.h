#ifndef TABLEDOUBLESPINR_H
#define TABLEDOUBLESPINR_H

#include "delegate/styleditemdelegate.h"

class TableDoubleSpinR final : public StyledItemDelegate {
public:
    explicit TableDoubleSpinR(const int& decimal, bool show_zero, QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int& decimal_;
    bool show_zero_ {};
};

#endif // TABLEDOUBLESPINR_H
