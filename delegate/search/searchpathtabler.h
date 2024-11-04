#ifndef SEARCHPATHTABLER_H
#define SEARCHPATHTABLER_H

// read only

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class SearchPathTableR final : public StyledItemDelegate {
public:
    SearchPathTableR(CTreeModel* model, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CTreeModel* model_ {};
};

#endif // SEARCHPATHTABLER_H
