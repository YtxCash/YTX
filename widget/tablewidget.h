#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include <QTableView>
#include <QWidget>

#include "table/model/tablemodel.h"

namespace Ui {
class TableWidget;
}

class TableWidget final : public QWidget {
    Q_OBJECT

public:
    explicit TableWidget(QWidget* parent = nullptr);
    ~TableWidget();

    void SetModel(TableModel* model);

    TableModel* Model() { return model_; }
    QTableView* View();

private:
    Ui::TableWidget* ui;
    TableModel* model_ {};
};

using TableHash = QHash<int, TableWidget*>;
using CTableHash = const QHash<int, TableWidget*>;

#endif // TABLEWIDGET_H
