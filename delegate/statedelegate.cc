#include "statedelegate.h"
#include <QApplication>
#include <QCheckBox>
#include <QMouseEvent>
StateDelegate::StateDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

QWidget* StateDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto check_box = new QCheckBox(parent);
    return check_box;
}

void StateDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto check_box = qobject_cast<QCheckBox*>(editor);
    if (check_box)
        check_box->setChecked(index.data().toBool());
}

void StateDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto check_box = qobject_cast<QCheckBox*>(editor);
    if (check_box)
        model->setData(index, check_box->isChecked());
}

void StateDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    auto opt = option;
    opt.rect.setWidth(opt.rect.width() / 2);
    opt.rect.moveCenter(option.rect.center());

    editor->setGeometry(opt.rect);
}

void StateDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    bool is_checked = index.data().toBool();

    auto opt = option;
    opt.rect.setWidth(opt.rect.width() / 2);
    opt.rect.moveCenter(option.rect.center());

    QStyleOptionButton check_box {};
    check_box.rect = opt.rect;
    check_box.state = is_checked ? QStyle::State_On : QStyle::State_Off;

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &check_box, painter);
}

bool StateDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (option.rect.contains(mouse_event->pos())) {
            bool is_checked = !index.data().toBool();
            model->setData(index, is_checked);
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}
