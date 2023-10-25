#include "textdelegate.h"
#include <QLineEdit>

TextDelegate::TextDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

QWidget* TextDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto line_editor = new QLineEdit(parent);
    return line_editor;
}

void TextDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);
    if (line_editor)
        line_editor->setText(index.data().toString());
}

void TextDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

void TextDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);
    if (line_editor)
        model->setData(index, line_editor->text());
}
