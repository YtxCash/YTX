#ifndef TREEAMOUNTR_H
#define TREEAMOUNTR_H

// read only

#include <QLocale>
#include <QStyledItemDelegate>

#include "component/using.h"

class TreeAmountR final : public QStyledItemDelegate {
public:
    TreeAmountR(const int* decimal, CStringHash* unit_symbol_hash, const int* base_unit, QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString FormattedString(const QModelIndex& index) const;

private:
    const int* decimal_ {};
    CStringHash* unit_symbol_hash_ {};
    const int* base_unit_ {};
    QLocale locale_ {};
};

#endif // TREEAMOUNTR_H
