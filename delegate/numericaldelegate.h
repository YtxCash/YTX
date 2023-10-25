#ifndef NUMERICALDELEGATE_H
#define NUMERICALDELEGATE_H

#include <QStyledItemDelegate>

class NumericalDelegate : public QStyledItemDelegate {
public:
    NumericalDelegate(int decimal, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    int decimal_ { 0 };
};

#endif // NUMERICALDELEGATE_H
