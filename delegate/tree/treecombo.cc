#include "treecombo.h"

#include <widget/combobox.h>

#include "component/enumclass.h"

TreeCombo::TreeCombo(CStringMap& map, bool skip_branch, QObject* parent)
    : StyledItemDelegate { parent }
    , map_ { map }
    , skip_branch_ { skip_branch }
{
    model_ = new QStandardItemModel(this);
    for (auto it = map_.cbegin(); it != map_.cend(); ++it) {
        auto* item { new QStandardItem(it.value()) };
        item->setData(it.key(), Qt::UserRole);
        model_->appendRow(item);
    }
}

QWidget* TreeCombo::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    if (skip_branch_ && index.siblingAtColumn(std::to_underlying(TreeEnum::kBranch)).data().toBool())
        return nullptr;

    auto* editor { new ComboBox(parent) };
    editor->setModel(model_);

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
    PaintText(MapValue(index.data().toInt()), painter, option, index, Qt::AlignCenter);
}

QSize TreeCombo::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    const QString text = MapValue(index.data().toInt());
    return CalculateTextSize(text, option);
}

void TreeCombo::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize text_size { CalculateTextSize(index.data().toString(), option) };

    // 取 option.rect 和 text_size 的宽度和高度的最大值
    int width { std::max(option.rect.width(), text_size.width()) };
    int height { std::max(option.rect.height(), text_size.height()) };

    editor->setFixedHeight(height);
    editor->setMinimumWidth(width);

    // 设置编辑器的几何位置和尺寸
    editor->setGeometry(option.rect);
}

QString TreeCombo::MapValue(int key) const
{
    static const QString empty_string {};
    auto it { map_.constFind(key) };

    return (it != map_.constEnd()) ? it.value() : empty_string;
}
