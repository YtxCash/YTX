#include "searchcombor.h"

#include "component/enumclass.h"

SearchComboR::SearchComboR(const TreeModel* model, QObject* parent)
    : StyledItemDelegate { parent }
    , model_ { model }
{
}

void SearchComboR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize SearchComboR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    return CalculateTextSize(path, option);
}

QString SearchComboR::GetPath(const QModelIndex& index) const
{
    return model_->GetPath(index.siblingAtColumn(std::to_underlying(TreeEnum::kID)).data().toInt());
}
