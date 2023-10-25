#include "searchcombor.h"

#include <QPainter>

#include "component/enumclass.h"

SearchComboR::SearchComboR(const TreeModel& model, QObject* parent)
    : QStyledItemDelegate { parent }
    , model_ { model }
{
}

void SearchComboR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    painter->setPen(option.state & QStyle::State_Selected ? option.palette.color(QPalette::HighlightedText) : option.palette.color(QPalette::Text));
    painter->drawText(option.rect.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize SearchComboR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    return path.isEmpty() ? QSize() : QSize(QFontMetrics(option.font).horizontalAdvance(path) + 8, option.rect.height());
}

QString SearchComboR::GetPath(const QModelIndex& index) const
{
    return model_.GetPath(index.sibling(index.row(), std::to_underlying(TreeEnum::kID)).data().toInt());
}
