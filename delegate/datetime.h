#ifndef DATETIME_H
#define DATETIME_H

#include <QDateTimeEdit>

#include "delegate/styleditemdelegate.h"

class DateTime final : public StyledItemDelegate {
public:
    DateTime(const QString& date_format, bool skip_branch, QObject* parent);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const QString& date_format_;
    mutable QDateTime last_date_time_ {};
    bool skip_branch_ {};
};

#endif // DATETIME_H
