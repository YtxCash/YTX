#ifndef STYLEDITEMDELEGATE_H
#define STYLEDITEMDELEGATE_H

#include <QLocale>
#include <QStyledItemDelegate>

#include "component/using.h"

class StyledItemDelegate : public QStyledItemDelegate {
public:
    explicit StyledItemDelegate(QObject* parent = nullptr);
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    static const QStyle* GetStyle(const QStyleOptionViewItem& opt);
    static QSize CalculateTextSize(CString& text, const QStyleOptionViewItem& option);

    void PaintText(CString& text, QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index, Qt::Alignment alignment) const;
    void PaintCheckBox(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

protected:
    static const QLocale locale_;
};

#endif // STYLEDITEMDELEGATE_H
