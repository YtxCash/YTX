#include "deadline.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/datetimeedit.h"

DeadLine::DeadLine(const QString& date_format, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
{
}

QWidget* DeadLine::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    if (Skip(index))
        return nullptr;

    auto* editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(date_format_);
    return editor;
}

void DeadLine::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto date_time { QDateTime::fromString(index.data().toString(), DATE_TIME_FST) };
    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    cast_ediotr->setDateTime(date_time);
}

void DeadLine::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    const auto& date_time { cast_ediotr->dateTime() };
    model->setData(index, date_time.toString(DATE_TIME_FST));
}

void DeadLine::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto date_time { index.data().toDateTime().toString(date_format_) };
    if (date_time.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(date_time, painter, option, index, Qt::AlignCenter);
}

QSize DeadLine::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const { return CalculateTextSize(DD, option); }

bool DeadLine::Skip(const QModelIndex& index) const
{
    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kBranch)).data().toBool() };
    const int unit { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kUnit)).data().toInt() };
    const bool rule { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kRule)).data().toBool() };

    return branch || unit == UNIT_PRODUCT || rule == RULE_CASH;
}
