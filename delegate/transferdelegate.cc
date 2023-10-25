#include "transferdelegate.h"
#include <QComboBox>
#include <QCompleter>
#include <QKeyEvent>
#include <QPainter>

TransferDelegate::TransferDelegate(const QMultiMap<QString, int>& leaf_map, QObject* parent)
    : QStyledItemDelegate { parent }
    , leaf_map_ { leaf_map }
{
}

QWidget* TransferDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto combobox = new QComboBox(parent);

    for (auto it = leaf_map_.cbegin(); it != leaf_map_.cend(); ++it) {
        QString path = it.key();
        int id = it.value();

        combobox->addItem(path, id);
    }
    combobox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    combobox->setFrame(false);
    combobox->setEditable(true);
    combobox->setInsertPolicy(QComboBox::NoInsert);
    combobox->setCurrentIndex(-1);

    auto completer = new QCompleter(combobox->model());
    completer->setFilterMode(Qt::MatchContains);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    combobox->setCompleter(completer);

    return combobox;
}

void TransferDelegate::ReceiveLeaf(const QMultiMap<QString, int>& leaf_map)
{
    leaf_map_.clear();
    leaf_map_ = leaf_map;
}

void TransferDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto combobox = qobject_cast<QComboBox*>(editor);

    int id = index.data().toInt();
    int item_index = combobox->findData(id);

    if (item_index != -1) {
        combobox->setCurrentIndex(item_index);
    }
}

void TransferDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);

    auto rect = option.rect;
    editor->setGeometry(rect);
}

void TransferDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto combobox = qobject_cast<QComboBox*>(editor);

    auto item_index = combobox->currentIndex();
    int id = combobox->itemData(item_index).toInt();

    model->setData(index, id);
}

void TransferDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int id = index.data().toInt();
    QString path {};

    for (auto it = leaf_map_.cbegin(); it != leaf_map_.cend(); ++it) {
        if (it.value() == id) {
            path = it.key();
            break;
        }
    }

    if (!path.isEmpty())
        painter->drawText(option.rect, Qt::AlignVCenter | Qt::AlignLeft, path);
    else
        QStyledItemDelegate::paint(painter, option, index);
}
