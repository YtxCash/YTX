#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QTreeView>
#include <QWidget>

#include "tree/model/treemodel.h"

class TreeWidget : public QWidget {
    Q_OBJECT

public slots:
    virtual void RUpdateDSpinBox() = 0;

public:
    virtual ~TreeWidget() = default;

    virtual void SetCurrentIndex(const QModelIndex& index) = 0;
    virtual void SetStatus() = 0;
    virtual QTreeView* View() = 0;
    virtual TreeModel* Model() = 0;

protected:
    TreeWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

#endif // TREEWIDGET_H
