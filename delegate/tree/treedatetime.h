#ifndef TREEDATETIME_H
#define TREEDATETIME_H

#include <QDateTimeEdit>

#include "delegate/styleditemdelegate.h"

class TreeDateTime final : public StyledItemDelegate {
public:
    TreeDateTime(const QString& date_format, bool skip_branch, QObject* parent);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const QString& date_format_;
    bool skip_branch_ {};
};

#endif // TREEDATETIME_H
