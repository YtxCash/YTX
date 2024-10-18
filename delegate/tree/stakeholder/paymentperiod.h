#ifndef PAYMENTPERIOD_H
#define PAYMENTPERIOD_H

#include "delegate/styleditemdelegate.h"

class PaymentPeriod final : public StyledItemDelegate {
public:
    PaymentPeriod(int min, int max, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    bool Skip(const QModelIndex& index) const;

private:
    int max_ {};
    int min_ {};
};

#endif // PAYMENTPERIOD_H
