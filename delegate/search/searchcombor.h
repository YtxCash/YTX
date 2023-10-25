#ifndef SEARCHCOMBOR_H
#define SEARCHCOMBOR_H

// read only

#include <QStyledItemDelegate>

#include "tree/model/treemodel.h"

class SearchComboR final : public QStyledItemDelegate {
public:
    SearchComboR(const TreeModel& model, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const TreeModel& model_;
};

#endif // SEARCHCOMBOR_H
