#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "delegate/styleditemdelegate.h"

class CheckBox final : public StyledItemDelegate {
public:
    explicit CheckBox(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    inline bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
};

#endif // CHECKBOX_H
