#include "tablecombo.h"

#include <QFontMetrics>
#include <QPainter>

#include "widget/combobox.h"

TableCombo::TableCombo(const TreeModel& model, int exclude, QObject* parent)
    : QStyledItemDelegate { parent }
    , exclude_ { exclude }
    , model_ { model }
{
}

QWidget* TableCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

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

void TableCombo::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}

void TableCombo::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    int key { qobject_cast<ComboBox*>(editor)->currentData().toInt() };
    last_insert_ = key;
    model->setData(index, key);
}

void TableCombo::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QString path { model_.GetPath(index.data().toInt()) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    painter->setPen(option.state & QStyle::State_Selected ? option.palette.color(QPalette::HighlightedText) : option.palette.color(QPalette::Text));
    painter->drawText(option.rect.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, path);
}

QSize TableCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QString path { model_.GetPath(index.data().toInt()) };
    return path.isEmpty() ? QSize() : QSize(QFontMetrics(option.font).horizontalAdvance(path) + 8, option.rect.height());
}
