#ifndef TREESPINR_H
#define TREESPINR_H

#include "delegate/styleditemdelegate.h"

class TreeSpinR final : public StyledItemDelegate {
public:
    TreeSpinR(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // TREESPINR_H
