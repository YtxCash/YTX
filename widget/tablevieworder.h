#ifndef TABLEVIEWORDER_H
#define TABLEVIEWORDER_H

#include <QTableView>

// enter 添加新行不实用，还是主动控制添加好

class TableViewOrder : public QTableView {
    Q_OBJECT
public:
    explicit TableViewOrder(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // TABLEVIEWORDER_H
