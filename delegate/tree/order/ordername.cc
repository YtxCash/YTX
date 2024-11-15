#include "ordername.h"

#include <QPainter>

#include "widget/combobox.h"

OrderName::OrderName(CTreeModel* tree_model, int unit, Filter filter, QObject* parent)
    : StyledItemDelegate { parent }
    , tree_model_ { tree_model }
    , filter_ { filter }
    , unit_ { unit }
{
    combo_model_ = new QStandardItemModel(this);
    RUpdateComboModel();
}

QWidget* OrderName::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    if (index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool())
        return nullptr;

    auto* editor { new ComboBox(parent) };
    editor->setModel(combo_model_);

    int height = option.rect.height();
    int width = option.rect.width();
    editor->setFixedHeight(std::max(height, editor->height()));
    editor->setMinimumWidth(std::max(width, editor->width()));

    return editor;
}

void OrderName::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int key { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt() };
    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void OrderName::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    int key { cast_editor->currentData().toInt() };
    model->setData(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)), key);
}

void OrderName::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString& text { tree_model_->GetPath(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt()) };
    if (text.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(text, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize OrderName::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
    const QString& text = index.data().toString() + tree_model_->GetPath(index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kParty)).data().toInt());
    return CalculateTextSize(text);
}

void OrderName::RUpdateComboModel() { tree_model_->LeafPathSpecificUnitPS(combo_model_, unit_, filter_); }
