#ifndef CHECKBOXR_H
#define CHECKBOXR_H

#include <QEvent>

#include "delegate/styleditemdelegate.h"

class CheckBoxR final : public StyledItemDelegate {
public:
    explicit CheckBoxR(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // CHECKBOXR_H
