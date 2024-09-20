#ifndef ORDERBRANCH_H
#define ORDERBRANCH_H

#include "delegate/styleditemdelegate.h"

class OrderBranch final : public StyledItemDelegate {
public:
    explicit OrderBranch(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    inline bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
};

#endif // ORDERBRANCH_H
