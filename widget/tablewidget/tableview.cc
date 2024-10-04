#include "tableview.h"

#include <QKeyEvent>
#include <QTimer>

#include "table/model/tablemodel.h"

TableView::TableView(QWidget* parent)
    : QTableView { parent }
{
}

void TableView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        auto* model { dynamic_cast<TableModel*>(this->model()) };
        if (model) {
            constexpr int ID_ZERO = 0;
            const int empty_row = model->GetRow(ID_ZERO);

            QModelIndex target_index {};

            if (empty_row == -1) {
                const int new_row = model->rowCount();
                if (!model->insertRows(new_row, 1))
                    return;

                target_index = model->index(new_row, std::to_underlying(TableEnum::kDateTime));
            } else {
                target_index = model->index(empty_row, std::to_underlying(TableEnum::kRelatedNode));
            }

            QTimer::singleShot(0, this, [=, this]() { this->setCurrentIndex(target_index); });
        }
        return; // 消费掉事件，防止进一步的处理
    }

    return QTableView::keyPressEvent(event); // 调用父类的事件过滤器
}
