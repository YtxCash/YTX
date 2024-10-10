#include "specificunit.h"

#include <QPainter>

#include "widget/combobox.h"

SpecificUnit::SpecificUnit(const TreeModel* tree_model, int specific_unit, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
    , specific_unit_ { specific_unit }
{
}

QWidget* SpecificUnit::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    Q_UNUSED(option);

    auto editor { new ComboBox(parent) };
    tree_model_->LeafPathSpecificUnit(editor, specific_unit_);
    editor->model()->sort(0);

    int height = option.rect.height();
    int width = option.rect.width();
    editor->setFixedHeight(std::max(height, editor->height()));
    editor->setMinimumWidth(std::max(width, editor->width()));

    return editor;
}

void SpecificUnit::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int item_index { cast_editor->findData(index.data().toInt()) };
    cast_editor->setCurrentIndex(item_index);
}

void SpecificUnit::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    model->setData(index, cast_editor->currentData().toInt());
}

void SpecificUnit::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize SpecificUnit::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(GetPath(index), option); }

QString SpecificUnit::GetPath(const QModelIndex& index) const { return tree_model_->GetPath(index.data().toInt()); }
