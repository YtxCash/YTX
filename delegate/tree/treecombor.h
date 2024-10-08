#ifndef TREECOMBOR_H
#define TREECOMBOR_H

#include "component/using.h"
#include "delegate/styleditemdelegate.h"

class TreeComboR final : public StyledItemDelegate {
public:
    TreeComboR(CStringHash& hash, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString HashValue(int key) const;

private:
    CStringHash& hash_;
};

#endif // TREECOMBOR_H
