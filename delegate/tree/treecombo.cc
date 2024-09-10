#include "treecombo.h"

#include <QtWidgets/qapplication.h>
#include <widget/combobox.h>

#include <QPainter>

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

    for (auto it = hash_.cbegin(); it != hash_.cend(); ++it)
        editor->addItem(it.value(), it.key());

    editor->model()->sort(0);
    return editor;
}

void TreeCombo::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<ComboBox*>(editor) };

    int item_index { cast_editor->findData(index.data().toInt()) };
    if (item_index != -1)
        cast_editor->setCurrentIndex(item_index);
}

void TreeCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    model->setData(index, qobject_cast<ComboBox*>(editor)->currentData().toInt());
}

void TreeCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    PaintItem(HashValue(index.data().toInt()), painter, option, Qt::AlignCenter);
}

QSize TreeCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = HashValue(index.data(Qt::EditRole).toInt());
    return CalculateSize(text, option, index);
}

QString TreeCombo::HashValue(int key) const
{
    static const QString empty_string {};
    auto it { hash_.constFind(key) };

    return (it != hash_.constEnd()) ? *it : empty_string;
}
