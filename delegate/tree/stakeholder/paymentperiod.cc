#include "paymentperiod.h"

#include "component/constvalue.h"
#include "component/enumclass.h"
#include "widget/spinbox.h"

PaymentPeriod::PaymentPeriod(int min, int max, QObject* parent)
    : StyledItemDelegate { parent }
    , max_ { max }
    , min_ { min }
{
}

QWidget* PaymentPeriod::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    if (Skip(index))
        return nullptr;

    auto* editor { new SpinBox(parent) };
    editor->setMinimum(min_);
    editor->setMaximum(max_);
    editor->setAlignment(Qt::AlignCenter);
    editor->setButtonSymbols(QAbstractSpinBox::NoButtons);

    return editor;
}

void PaymentPeriod::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<SpinBox*>(editor) };
    cast_editor->setValue(index.data().toInt());
}

void PaymentPeriod::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<SpinBox*>(editor) };
    model->setData(index, cast_editor->value());
}

void PaymentPeriod::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value), painter, option, index, Qt::AlignCenter);
}

QSize PaymentPeriod::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    const int value { index.data().toInt() };
    return CalculateTextSize(locale_.toString(value));
}

void PaymentPeriod::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/*index*/) const
{
    editor->setFixedHeight(option.rect.height());
    editor->setFixedWidth(option.rect.width());

           // 设置编辑器的几何位置和尺寸
    editor->setGeometry(option.rect);
}

bool PaymentPeriod::Skip(const QModelIndex& index) const
{
    const bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kBranch)).data().toBool() };
    const int unit { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kUnit)).data().toInt() };
    const bool rule { index.siblingAtColumn(std::to_underlying(TreeEnumStakeholder::kRule)).data().toBool() };

    return branch || unit == UNIT_PROD || rule == RULE_IM;
}
