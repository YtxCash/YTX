#include "styleditemdelegate.h"

#include <QApplication>
#include <QFontMetrics>

StyledItemDelegate::StyledItemDelegate(QObject* parent)
    : QStyledItemDelegate { parent }
    , locale_ { QLocale::English, QLocale::UnitedStates }
{
}

void StyledItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& /*index*/) const
{
    editor->setGeometry(option.rect);
}

const QStyle* StyledItemDelegate::GetStyle(const QStyleOptionViewItem& opt) const { return opt.widget ? opt.widget->style() : QApplication::style(); }

QSize StyledItemDelegate::CalculateSize(CString& text, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    auto style { GetStyle(opt) };
    const int text_margin { style->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, opt.widget) + 1 };

    const QFontMetrics fm(opt.font);
    const int width = std::max(fm.horizontalAdvance(text) + 4 * text_margin, opt.rect.width());
    const int height = std::max(fm.height(), opt.rect.height());

    return QSize(width, height);
}

void StyledItemDelegate::PaintItem(CString& text, QPainter* painter, const QStyleOptionViewItem& option, Qt::Alignment alignment) const
{
    QStyleOptionViewItem opt(option);
    opt.displayAlignment = alignment;

    opt.text = text;

    auto style { GetStyle(opt) };
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
}
