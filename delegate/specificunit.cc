#include "specificunit.h"

#include <QPainter>

#include "widget/combobox.h"

SpecificUnit::SpecificUnit(const TreeModel* tree_model, int unit, UnitFilterMode unit_filter_mode, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
    , unit_filter_mode_ { unit_filter_mode }
    , unit_ { unit }
{
}

QWidget* SpecificUnit::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    Q_UNUSED(option);

    auto* editor { new ComboBox(parent) };

    switch (unit_filter_mode_) {
    case UnitFilterMode::kIncludeUnitOnly:
        tree_model_->LeafPathIncludeUnitOnly(editor, unit_);
        break;
    case UnitFilterMode::kExcludeUnitOnly:
        tree_model_->LeafPathExcludeUnitOnly(editor, unit_);
        break;
    default:
        break;
    }

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
    int key { index.data().toInt() };
    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void SpecificUnit::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int key { cast_editor->currentData().toInt() };
    model->setData(index, key);
}

void SpecificUnit::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text { tree_model_->GetPath(index.data().toInt()) };
    if (text.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize SpecificUnit::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text = tree_model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}
