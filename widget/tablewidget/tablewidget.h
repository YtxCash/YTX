#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QWidget>

#include "table/model/tablemodel.h"
#include "widget/tablewidget/tableview.h"

class TableWidget : public QWidget {
    Q_OBJECT

public:
    virtual ~TableWidget() = default;

    virtual void SetModel(TableModel* model) = 0;
    virtual TableModel* Model() = 0;
    virtual TableView* View() = 0;

protected:
    explicit TableWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using TableHash = QHash<int, TableWidget*>;
using CTableHash = const QHash<int, TableWidget*>;

#endif // TABLEWIDGET_H
