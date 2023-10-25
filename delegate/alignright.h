#ifndef ALIGNRIGHT_H
#define ALIGNRIGHT_H

#include <QStyledItemDelegate>

class AlignRight : public QStyledItemDelegate {
public:
    explicit AlignRight(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // ALIGNRIGHT_H
