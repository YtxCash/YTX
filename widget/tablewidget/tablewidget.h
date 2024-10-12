#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QTableView>
#include <QWidget>

#include "table/model/tablemodel.h"

class TableWidget : public QWidget {
    Q_OBJECT

public:
    virtual ~TableWidget() = default;

    virtual TableModel* Model() = 0;
    virtual QTableView* View() = 0;

protected:
    explicit TableWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using TableHash = QHash<int, TableWidget*>;
using CTableHash = const QHash<int, TableWidget*>;

#endif // TABLEWIDGET_H
