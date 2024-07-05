#include "employeer.h"

#include <QPainter>

EmployeeR::EmployeeR(CStringHash* stakeholder_branch_path, QObject* parent)
    : QStyledItemDelegate { parent }
    , stakeholder_branch_path_ { stakeholder_branch_path }
{
}

void EmployeeR::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    auto selected { option.state & QStyle::State_Selected };
    auto palette { option.palette };

    painter->setPen(selected ? palette.color(QPalette::HighlightedText) : palette.color(QPalette::Text));
    if (selected)
        painter->fillRect(option.rect, palette.highlight());

    painter->drawText(option.rect.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize EmployeeR::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    return path.isEmpty() ? QSize() : QSize(QFontMetrics(option.font).horizontalAdvance(path) + 8, option.rect.height());
}

QString EmployeeR::GetPath(const QModelIndex& index) const { return stakeholder_branch_path_->value(index.data().toInt(), QString()); }
