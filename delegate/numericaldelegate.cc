#include "numericaldelegate.h"
#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

NumericalDelegate::NumericalDelegate(int decimal, QObject* parent)
    : QStyledItemDelegate { parent }
    , decimal_ { decimal }
{
}

QWidget* NumericalDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto line_editor = new QLineEdit(parent);

    QString re_pattern = QString("[-+]?\\d*(\\.\\d{0,%1})?").arg(decimal_);
    QRegularExpression re(re_pattern);

    auto validator = new QRegularExpressionValidator(re, line_editor);
    line_editor->setValidator(validator);

    return line_editor;
}

void NumericalDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);

    double value = index.data().toDouble();
    line_editor->setText(QString::number(value, 'f', decimal_));
}

void NumericalDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);

    editor->setGeometry(option.rect);
}

void NumericalDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto line_editor = qobject_cast<QLineEdit*>(editor);

    auto value = line_editor->text().toDouble();
    model->setData(index, value);
}

void NumericalDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto opt = option;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
    QStyledItemDelegate::paint(painter, opt, index);
}
