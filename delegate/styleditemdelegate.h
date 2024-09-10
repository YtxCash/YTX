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
    const QStyle* GetStyle(const QStyleOptionViewItem& opt) const;

    QSize CalculateSize(CString& text, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void PaintItem(CString& text, QPainter* painter, const QStyleOptionViewItem& option, Qt::Alignment alignment) const;

protected:
    QLocale locale_ {};
};

#endif // STYLEDITEMDELEGATE_H
