#ifndef DATEDELEGATE_H
#define DATEDELEGATE_H

#include <QStyledItemDelegate>

class DateDelegate : public QStyledItemDelegate {
public:
    DateDelegate(const QString& format, QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QString GetSeparator(const QString& format);
    void LastMonthEnd(QDate& date);
    void NextMonthStart(QDate& date);

private:
    QString separator_ {};
    QString format_ {};
};

#endif // DATEDELEGATE_H
