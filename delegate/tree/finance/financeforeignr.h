#ifndef FINANCEFOREIGNR_H
#define FINANCEFOREIGNR_H

// read only

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class FinanceForeignR final : public StyledItemDelegate {
public:
    FinanceForeignR(const int& decimal, const int& base_unit, CStringHash& unit_symbol_hash, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QModelIndex& index) const;

private:
    const int& decimal_;
    const int& base_unit_;
    CStringHash& unit_symbol_hash_;
};

#endif // FINANCEFOREIGNR_H
