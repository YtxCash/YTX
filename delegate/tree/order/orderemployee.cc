#include "orderemployee.h"

#include <QPainter>

#include "widget/combobox.h"

OrderEmployee::OrderEmployee(const AbstractTreeModel& modstakeholder_tree_model, QObject* parent)
    : StyledItemDelegate { parent }
    , stakeholder_tree_model_ { modstakeholder_tree_model }
{
}

QWidget* OrderEmployee::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    bool branch { index.siblingAtColumn(std::to_underlying(TreeEnumOrder::kBranch)).data().toBool() };
    if (branch)
        return nullptr;

    auto editor { new ComboBox(parent) };
    stakeholder_tree_model_.ComboPathUnit(editor, UNIT_EMPLOYEE);
    editor->model()->sort(0);

    return editor;
}

void OrderEmployee::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto cast_editor { qobject_cast<ComboBox*>(editor) };
    int item_index { cast_editor->findData(index.data().toInt()) };
    cast_editor->setCurrentIndex(item_index);
}

void OrderEmployee::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    model->setData(index, qobject_cast<ComboBox*>(editor)->currentData().toInt());
}

void OrderEmployee::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto path { GetPath(index) };
    if (path.isEmpty())
        return QStyledItemDelegate::paint(painter, option, index);

    PaintItem(path, painter, option, Qt::AlignLeft | Qt::AlignVCenter);
}

QSize OrderEmployee::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const { return CalculateSize(GetPath(index), option, index); }

QString OrderEmployee::GetPath(const QModelIndex& index) const { return stakeholder_tree_model_.GetPath(index.data().toInt()); }
