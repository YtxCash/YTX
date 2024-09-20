#ifndef ORDERDATETIME_H
#define ORDERDATETIME_H

#include <QDateTime>

#include "delegate/styleditemdelegate.h"

class OrderDateTime final : public StyledItemDelegate {
public:
    explicit OrderDateTime(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // ORDERDATETIME_H
