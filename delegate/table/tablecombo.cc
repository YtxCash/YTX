#include "tablecombo.h"

#include "widget/combobox.h"

TableCombo::TableCombo(const TreeModel* tree_model, int exclude_id, QObject* parent)
    : StyledItemDelegate { parent }
    , exclude_id_ { exclude_id }
    , tree_model_ { tree_model }
{
    combo_model_ = new QStandardItemModel(this);
    RUpdateComboModel();
}

QWidget* TableCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    auto editor { new ComboBox(parent) };
    editor->setModel(combo_model_);

    int height = option.rect.height();
    int width = option.rect.width();
    editor->setFixedHeight(std::max(height, editor->height()));
    editor->setMinimumWidth(std::max(width, editor->width()));
    return editor;
}

void TableCombo::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { static_cast<ComboBox*>(editor) };

    int key { index.data().toInt() };
    if (key == 0)
        key = last_insert_;

    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void TableCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto cast_editor { static_cast<ComboBox*>(editor) };

    int key { cast_editor->currentData().toInt() };
    last_insert_ = key;
    model->setData(index, key);
}

void TableCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString path { tree_model_->GetPath(index.data().toInt()) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize TableCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = tree_model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}

void TableCombo::RUpdateComboModel() { tree_model_->LeafPathExcludeID(combo_model_, exclude_id_); }
