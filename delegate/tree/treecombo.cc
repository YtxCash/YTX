#include "treecombo.h"

#include <widget/combobox.h>

TreeCombo::TreeCombo(CStringHash& hash, QObject* parent)
    : StyledItemDelegate { parent }
    , hash_ { hash }
{
}

QWidget* TreeCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto editor { new ComboBox(parent) };

    editor->blockSignals(true);

    for (auto it = hash_.cbegin(); it != hash_.cend(); ++it)
        editor->addItem(it.value(), it.key());

    editor->model()->sort(0);
    editor->blockSignals(false);

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
    const QString text = HashValue(index.data(Qt::EditRole).toInt());
    return CalculateTextSize(text, option);
}

QString TreeCombo::HashValue(int key) const
{
    static const QString empty_string {};
    auto it { hash_.constFind(key) };

    return (it != hash_.constEnd()) ? it.value() : empty_string;
}
