#ifndef DEADLINE_H
#define DEADLINE_H

#include <QDateTimeEdit>

#include "delegate/styleditemdelegate.h"

class DeadLine final : public StyledItemDelegate {
public:
    DeadLine(const QString& date_format, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    bool SkipDeadline(const QModelIndex& index) const;

private:
    QString date_format_;
};

#endif // DEADLINE_H
