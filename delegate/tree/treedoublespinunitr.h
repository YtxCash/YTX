#ifndef TREEDOUBLESPINUNITR_H
#define TREEDOUBLESPINUNITR_H

// read only

#include <QLocale>
#include <QStyledItemDelegate>

#include "component/using.h"

class TreeDoubleSpinUnitR final : public QStyledItemDelegate {
public:
    TreeDoubleSpinUnitR(const int& decimal, CStringHash& unit_symbol_hash, bool format_zero = true, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString Format(const QModelIndex& index) const;

private:
    const int& decimal_ {};
    bool format_zero_ { true };
    CStringHash& unit_symbol_hash_;
    QLocale locale_ {};
};

#endif // TREEDOUBLESPINUNITR_H
