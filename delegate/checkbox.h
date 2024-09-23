#ifndef CHECKBOX_H
#define CHECKBOX_H

#include <QEvent>

#include "delegate/styleditemdelegate.h"

class CheckBox final : public StyledItemDelegate {
public:
    explicit CheckBox(QEvent::Type type, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    inline bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QEvent::Type type_ {};
};

#endif // CHECKBOX_H
