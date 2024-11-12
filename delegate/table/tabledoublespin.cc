#include "tabledoublespin.h"

#include "widget/doublespinbox.h"

TableDoubleSpin::TableDoubleSpin(const int& decimal, double min, double max,bool max_width, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , max_ { max }
    , min_ { min }
    ,max_width_{max_width}
{
}

QWidget* TableDoubleSpin::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto* editor { new DoubleSpinBox(parent) };
    editor->setDecimals(decimal_);
    editor->setMinimum(min_);
    editor->setMaximum(max_);
    editor->setButtonSymbols(QAbstractSpinBox::NoButtons);

    int width = option.rect.width();
    int height = option.rect.height();
    editor->setFixedSize(width, height);

    return editor;
}

void TableDoubleSpin::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DoubleSpinBox*>(editor) };
    cast_editor->setValue(index.data().toDouble());
}

void TableDoubleSpin::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<DoubleSpinBox*>(editor) };
    model->setData(index, cast_editor->value());
}

void TableDoubleSpin::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const double value { index.data().toDouble() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(locale_.toString(value, 'f', decimal_), painter, option, index, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TableDoubleSpin::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    if(max_width_)
        return CalculateTextSize(locale_.toString(DMIN, 'f', decimal_));

    const double value { index.data().toDouble() };
    return CalculateTextSize(locale_.toString(value, 'f', decimal_));
}
