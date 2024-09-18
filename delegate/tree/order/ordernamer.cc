#include "ordernamer.h"

#include <QPainter>

#include "component/enumclass.h"

OrderName::OrderName(const AbstractTreeModel& model, QObject* parent)
    : StyledItemDelegate { parent }
    , model_ { model }
{
}

void OrderName::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize OrderName::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    return CalculateTextSize(path, option);
}

QString OrderName::GetPath(const QModelIndex& index) const
{
    int node_id { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt() };
    return node_id == 0 ? index.data().toString() : model_.GetPath(node_id);
}
