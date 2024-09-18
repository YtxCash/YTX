#ifndef ORDERLOCKED_H
#define ORDERLOCKED_H

// tree's branch column 6, table's state column 7, arte different, so they can share this delegate

#include "delegate/styleditemdelegate.h"

class OrderLocked final : public StyledItemDelegate {
public:
    explicit OrderLocked(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    inline bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
};

#endif // ORDERLOCKED_H
