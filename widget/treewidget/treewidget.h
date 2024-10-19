#ifndef TREEWIDGET_H
#define TREEWIDGET_H

#include <QPointer>
#include <QTreeView>
#include <QWidget>

#include "tree/model/treemodel.h"

class TreeWidget : public QWidget {
    Q_OBJECT

public slots:
    virtual void RUpdateDSpinBox() { };

public:
    virtual ~TreeWidget() = default;

    virtual void SetStatus() { };
    virtual QPointer<QTreeView> View() const = 0;
    virtual QPointer<TreeModel> Model() const = 0;

protected:
    TreeWidget(QWidget* parent = nullptr)
        : QWidget { parent }
    {
    }
};

using PQTreeView = QPointer<QTreeView>;

#endif // TREEWIDGET_H
