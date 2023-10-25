#ifndef TEXTDELEGATE_H
#define TEXTDELEGATE_H

#include <QStyledItemDelegate>

class TextDelegate : public QStyledItemDelegate {
public:
    explicit TextDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
};

#endif // TEXTDELEGATE_H
