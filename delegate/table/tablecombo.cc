#include "tablecombo.h"

#include <QFontMetrics>
#include <QPainter>

#include "widget/combobox.h"

TableCombo::TableCombo(const AbstractTreeModel& model, int exclude, QObject* parent)
    : StyledItemDelegate { parent }
    , exclude_ { exclude }
    , model_ { model }
{
}

QWidget* TableCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/, const QModelIndex& /*index*/) const
{
    auto editor { new ComboBox(parent) };
    model_.ComboPathLeaf(editor, exclude_);
    editor->model()->sort(0);
    return editor;
}

void TableCombo::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<ComboBox*>(editor) };

    int key { index.data().toInt() };
    if (key == 0)
        key = last_insert_;

    int item_index { cast_editor->findData(key) };
    cast_editor->setCurrentIndex(item_index);
}

void TableCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    int key { qobject_cast<ComboBox*>(editor)->currentData().toInt() };
    last_insert_ = key;
    model->setData(index, key);
}

void TableCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString path { model_.GetPath(index.data().toInt()) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(path, painter, option, Qt::AlignLeft | Qt::AlignVCenter);

    // 高度自定义
    // const int text_margin { CalculateTextMargin(style, opt.widget) };
    // const QRect text_rect { opt.rect.adjusted(text_margin, 0, -text_margin, 0) };
    // painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize TableCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = model_.GetPath(index.data().toInt());
    return CalculateSize(text, option, index);
}
