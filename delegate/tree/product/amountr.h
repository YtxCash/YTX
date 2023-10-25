#ifndef AMOUNTR_H
#define AMOUNTR_H

// read only

#include <QLocale>
#include <QStyledItemDelegate>

#include "component/using.h"

class AmountR final : public QStyledItemDelegate {
public:
    AmountR(const int& decimal, CStringHash& currency_symbol_hash, const int& base_currency, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString FormattedString(const QModelIndex& index) const;

private:
    const int& decimal_;
    CStringHash& currency_symbol_hash_;
    const int& base_currency_;
    QLocale locale_ {};
};

#endif // AMOUNTR_H
