#include "searchcombor.h"

#include <QPainter>

#include "component/enumclass.h"

SearchComboR::SearchComboR(const AbstractTreeModel& model, QObject* parent)
    : StyledItemDelegate { parent }
    , model_ { model }
{
}

void SearchComboR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(path, painter, option, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize SearchComboR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    return CalculateSize(path, option, index);
}

QString SearchComboR::GetPath(const QModelIndex& index) const
{
    return model_.GetPath(index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt());
}
