#include "treespin.h"

#include <QPainter>

#include "widget/spinbox.h"

TreeSpin::TreeSpin(int min, int max, QObject* parent)
    : StyledItemDelegate { parent }
    , max_ { max }
    , min_ { min }
{
}

QWidget* TreeSpin::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor { new SpinBox(parent) };
    editor->setMinimum(min_);
    editor->setMaximum(max_);
    editor->setAlignment(Qt::AlignCenter);

    return editor;
}

void TreeSpin::setEditorData(QWidget* editor, const QModelIndex& index) const { qobject_cast<SpinBox*>(editor)->setValue(index.data().toInt()); }

void TreeSpin::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<SpinBox*>(editor) };
    model->setData(index, cast_editor->value());
}

void TreeSpin::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toInt() };
    if (value == 0)
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(locale_.toString(value), painter, option, Qt::AlignCenter);
}

QSize TreeSpin::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto value { index.data().toInt() };
    return CalculateSize(locale_.toString(value), option, index);
}
