#ifndef SEARCHCOMBOR_H
#define SEARCHCOMBOR_H

// read only

#include "delegate/styleditemdelegate.h"
#include "tree/model/abstracttreemodel.h"

class SearchComboR final : public StyledItemDelegate {
public:
    SearchComboR(const AbstractTreeModel& model, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    const AbstractTreeModel& model_;
};

#endif // SEARCHCOMBOR_H
