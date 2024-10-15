#include "tablevieworder.h"

#include <QKeyEvent>
#include <QTimer>

#include "table/model/tablemodel.h"

TableViewOrder::TableViewOrder(QWidget* parent)
    : QTableView { parent }
{
}

void TableViewOrder::keyPressEvent(QKeyEvent* event)
{
    if (event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter) {
        QTableView::keyPressEvent(event);
        return;
    }

    assert(dynamic_cast<TableModel*>(this->model()) && "Model is not TableModel");
    auto* model { static_cast<TableModel*>(this->model()) };

    constexpr int ID_ZERO { 0 };
    const int target_row { [model]() {
        const int empty_row = model->GetNodeRow(ID_ZERO);
        return empty_row == -1 ? model->rowCount() : empty_row;
    }() };

    if (target_row == model->rowCount() && !model->insertRows(target_row, 1)) {
        qWarning() << "Failed to insert new row";
        return;
    }

    const QModelIndex target_index { model->index(target_row, std::to_underlying(TableEnumOrder::kInsideProduct)) };
    QTimer::singleShot(0, this, [=, this]() { this->setCurrentIndex(target_index); });
    event->accept();
}
