#include "treedatetime.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/datetimeedit.h"

TreeDateTime::TreeDateTime(const QString& date_format, bool skip_branch, QObject* parent)
    : StyledItemDelegate { parent }
    , date_format_ { date_format }
    , skip_branch_ { skip_branch }
{
}

QWidget* TreeDateTime::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    if (skip_branch_ && index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool())
        return nullptr;

    auto* editor { new DateTimeEdit(parent) };
    editor->setDisplayFormat(date_format_);

    return editor;
}

void TreeDateTime::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto date_time { QDateTime::fromString(index.data().toString(), DATE_TIME_FST) };
    if (!date_time.isValid())
        date_time = QDateTime::currentDateTime();

    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    cast_ediotr->setDateTime(date_time);
}

void TreeDateTime::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_ediotr { static_cast<DateTimeEdit*>(editor) };
    auto date_time { cast_ediotr->dateTime() };

    model->setData(index, date_time.toString(DATE_TIME_FST));
}

void TreeDateTime::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto date_time { index.data().toDateTime() };
    if (!date_time.isValid())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(date_time.toString(date_format_), painter, option, index, Qt::AlignCenter);
}

QSize TreeDateTime::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto text { index.data().toDateTime().toString(date_format_) };
    return CalculateTextSize(text, option);
}
