#include "stakeholderrhsnode.h"

#include "widget/combobox.h"

StakeholderRhsNode::StakeholderRhsNode(const TreeModel* model, int exclude_unit, QObject* parent)
    : StyledItemDelegate { parent }
    , exclude_unit_ { exclude_unit }
    , model_ { model }
{
}

QWidget* StakeholderRhsNode::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    auto editor { new ComboBox(parent) };
    model_->ComboPathLeafExcludeUnit(editor, exclude_unit_);
    editor->model()->sort(0);

    int height = option.rect.height();
    int width = option.rect.width();
    editor->setFixedHeight(std::max(height, editor->height()));
    editor->setMinimumWidth(std::max(width, editor->width()));
    return editor;
}

void StakeholderRhsNode::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<ComboBox*>(editor) };

    int key { index.data().toInt() };
    if (key == 0)
        key = last_insert_;

    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void StakeholderRhsNode::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    int key { qobject_cast<ComboBox*>(editor)->currentData().toInt() };
    last_insert_ = key;
    model->setData(index, key);
}

void StakeholderRhsNode::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString path { model_->GetPath(index.data().toInt()) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize StakeholderRhsNode::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = model_->GetPath(index.data().toInt());
    return CalculateTextSize(text, option);
}
