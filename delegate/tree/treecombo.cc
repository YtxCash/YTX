#include "treecombo.h"

#include <widget/combobox.h>

#include "component/enumclass.h"

TreeCombo::TreeCombo(CStringHash& hash, bool skip_branch, QObject* parent)
    : StyledItemDelegate { parent }
    , hash_ { hash }
    , skip_branch_ { skip_branch }
{
    model_ = new QStandardItemModel(this);
    for (auto it = hash_.cbegin(); it != hash_.cend(); ++it) {
        auto* item { new QStandardItem(it.value()) };
        item->setData(it.key(), Qt::UserRole);
        model_->appendRow(item);
    }

    model_->sort(0);
}

QWidget* TreeCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    if (skip_branch_ && index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool())
        return nullptr;

    auto* editor { new ComboBox(parent) };
    editor->setModel(model_);

    int height = option.rect.height();
    int width = option.rect.width();
    editor->setFixedHeight(std::max(height, editor->height()));
    editor->setMinimumWidth(std::max(width, editor->width()));

    return editor;
}

void TreeCombo::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };

    int item_index { cast_editor->findData(index.data().toInt()) };
    if (item_index != -1)
        cast_editor->setCurrentIndex(item_index);
}

void TreeCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto* cast_editor { static_cast<ComboBox*>(editor) };
    model->setData(index, cast_editor->currentData().toInt());
}

void TreeCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintText(HashValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

QSize TreeCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = HashValue(index.data().toInt());
    return CalculateTextSize(text, option);
}

QString TreeCombo::HashValue(int key) const
{
    static const QString empty_string {};
    auto it { hash_.constFind(key) };

    return (it != hash_.constEnd()) ? it.value() : empty_string;
}
