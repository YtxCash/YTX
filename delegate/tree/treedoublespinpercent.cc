#include "treedoublespinpercent.h"

#include "component/constvalue.h"
#include "widget/doublespinbox.h"

TreeDoubleSpinPercent::TreeDoubleSpinPercent(const int& decimal, double min, double max, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , max_ { max }
    , min_ { min }
{
}

QWidget* TreeDoubleSpinPercent::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor { new DoubleSpinBox(parent) };
    editor->setSuffix(SFX_PERCENT);
    editor->setDecimals(decimal_);
    editor->setMinimum(min_);
    editor->setMaximum(max_);

    return editor;
}

void TreeDoubleSpinPercent::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    qobject_cast<DoubleSpinBox*>(editor)->setValue(index.data().toDouble() * HUNDRED);
}

void TreeDoubleSpinPercent::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<DoubleSpinBox*>(editor) };
    model->setData(index, cast_editor->value() / HUNDRED);
}

void TreeDoubleSpinPercent::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() * HUNDRED };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_) + SFX_PERCENT, painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TreeDoubleSpinPercent::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() * HUNDRED };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_) + SFX_PERCENT, option);
}
