#ifndef TREEDOUBLESPIN_H
#define TREEDOUBLESPIN_H

#include "delegate/styleditemdelegate.h"

class TreeDoubleSpin : public StyledItemDelegate {
public:
    TreeDoubleSpin(const int& decimal, double min, double max, QObject* parent = nullptr);
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    const int& decimal_;
    double max_ {};
    double min_ {};
};

#endif // TREEDOUBLESPIN_H
