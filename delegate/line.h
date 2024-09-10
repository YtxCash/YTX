#ifndef LINE_H
#define LINE_H

#include "delegate/styleditemdelegate.h"

class Line final : public StyledItemDelegate {
public:
    explicit Line(QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};

#endif // LINE_H
