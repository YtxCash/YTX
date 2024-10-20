#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QPointer>
#include <QTableView>
#include <QWidget>

#include "table/model/tablemodel.h"

class TableWidget : public QWidget {
    Q_OBJECT

public:
    virtual ~TableWidget() = default;

    virtual QPointer<TableModel> Model() const = 0;
    virtual QPointer<QTableView> View() const = 0;

protected:
    explicit TableWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using TableHash = QHash<int, TableWidget*>;
using CTableHash = const QHash<int, TableWidget*>;
using PQTableView = QPointer<QTableView>;

#endif // TABLEWIDGET_H
