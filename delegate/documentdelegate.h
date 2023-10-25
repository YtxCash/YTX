#ifndef DOCUMENTDELEGATE_H
#define DOCUMENTDELEGATE_H

#include <QStyledItemDelegate>

class DocumentDelegate : public QStyledItemDelegate {
public:
    explicit DocumentDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // DOCUMENTDELEGATE_H
