#include "styleditemdelegate.h"

#include <QApplication>
#include <QFontMetrics>

const QLocale StyledItemDelegate::locale_ { QLocale::English, QLocale::UnitedStates };
std::optional<QFontMetrics> StyledItemDelegate::fm_ = std::nullopt;
std::optional<int> StyledItemDelegate::text_margin_ = std::nullopt;

StyledItemDelegate::StyledItemDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
{
}

void StyledItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    editor->setFixedHeight(option.rect.height());
    editor->setFixedWidth(option.rect.width());
    editor->setGeometry(option.rect);
}

void StyledItemDelegate::SetFontMetrics() { fm_ = QFontMetrics(QApplication::font()); }

void StyledItemDelegate::SetTextMargin() { text_margin_ = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 2; }

QSize StyledItemDelegate::CalculateTextSize(CString& text)
{
    const int width { fm_->horizontalAdvance(text) + 3 * text_margin_.value() };
    const int height { fm_->height() };

    return QSize(width, height);
}

void StyledItemDelegate::PaintText(
    CString& text, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, Qt::Alignment alignment) const
{
    QStyleOptionViewItem opt(option);
    opt.displayAlignment = alignment;

    initStyleOption(&opt, index);

    opt.text = text;

    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}

void StyledItemDelegate::PaintCheckBox(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt { option };
    initStyleOption(&opt, index);

    auto* style { QApplication::style() };
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    QStyleOptionButton check_box {};
    check_box.state = option.state;
    check_box.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;

    auto rect { style->subElementRect(QStyle::SE_CheckBoxIndicator, &check_box, opt.widget) };
    rect.moveCenter(opt.rect.center());
    check_box.rect = rect;

    style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &check_box, painter, option.widget);
}
