#include "orderstakeholder.h"

#include <QPainter>

#include "widget/combobox.h"

OrderStakeholder::OrderStakeholder(const TreeModel& modstakeholder_tree_model, int unit, QObject* parent)
    : StyledItemDelegate { parent }
    , stakeholder_tree_model_ { modstakeholder_tree_model }
    , unit_ { unit }
{
}

QWidget* OrderStakeholder::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool() };
    if (branch)
        return nullptr;

    auto editor { new ComboBox(parent) };
    stakeholder_tree_model_.ComboPathUnit(editor, unit_);
    editor->model()->sort(0);

    return editor;
}

void OrderStakeholder::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<ComboBox*>(editor) };
    int item_index { cast_editor->findData(index.data().toInt()) };
    cast_editor->setCurrentIndex(item_index);
}

void OrderStakeholder::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    model->setData(index, qobject_cast<ComboBox*>(editor)->currentData().toInt());
}

void OrderStakeholder::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintText(path, painter, option, index, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize OrderStakeholder::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateTextSize(GetPath(index), option); }

QString OrderStakeholder::GetPath(const QModelIndex& index) const { return stakeholder_tree_model_.GetPath(index.data().toInt()); }
