#ifndef DATETIME_H
#define DATETIME_H

// dynamic date time delegate, usually for table column

#include <QDateTimeEdit>
#include <QRegularExpression>

#include "delegate/styleditemdelegate.h"

class DateTime final : public StyledItemDelegate {
public:
    DateTime(const QString& date_format, const bool& hide_time, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QDateTime& date_time) const;

private:
    const QString& date_format_;
    const bool& hide_time_;
    QRegularExpression time_pattern_ {};
    mutable QDateTime last_date_time_ {};
};

#endif // DATETIME_H
