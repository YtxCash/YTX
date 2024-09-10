#ifndef TREEPLAINTEXT_H
#define TREEPLAINTEXT_H

#include "delegate/styleditemdelegate.h"

class TreePlainText final : public StyledItemDelegate {
public:
    explicit TreePlainText(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};

#endif // TREEPLAINTEXT_H
