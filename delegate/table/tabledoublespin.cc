#include "tabledoublespin.h"

#include <QPainter>

#include "widget/doublespinbox.h"

TableDoubleSpin::TableDoubleSpin(const int& decimal, double min, double max, QObject* parent)
    : StyledItemDelegate { parent }
    , decimal_ { decimal }
    , max_ { max }
    , min_ { min }
{
}

QWidget* TableDoubleSpin::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor { new DoubleSpinBox(parent) };
    editor->setDecimals(decimal_);
    editor->setMinimum(min_);
    editor->setMaximum(max_);

    return editor;
}

void TableDoubleSpin::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    qobject_cast<DoubleSpinBox*>(editor)->setValue(index.data().toDouble());
}

void TableDoubleSpin::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<DoubleSpinBox*>(editor) };
    model->setData(index, cast_editor->value());
}

void TableDoubleSpin::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(locale_.toString(value, 'f', decimal_), painter, option, Qt::AlignRight | Qt::AlignVCenter);
}

QSize TableDoubleSpin::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toDouble() };
    return CalculateSize(locale_.toString(value, 'f', decimal_), option, index);
}
