#ifndef SEARCHPATHBYCOLUMNR_H
#define SEARCHPATHBYCOLUMNR_H

// read only

#include "delegate/styleditemdelegate.h"
#include "tree/model/treemodel.h"

class SearchPathByColumnR final : public StyledItemDelegate {
public:
    SearchPathByColumnR(CTreeModel* model, int column, QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString GetPath(const QModelIndex& index) const;

private:
    CTreeModel* model_ {};
    int column_ {};
};

#endif // SEARCHPATHBYCOLUMNR_H
