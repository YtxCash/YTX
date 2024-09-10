#ifndef REMOVENODE_H
#define REMOVENODE_H

#include <QDialog>

#include "tree/model/abstracttreemodel.h"

namespace Ui {
class RemoveNode;
}

class RemoveNode final : public QDialog {
    Q_OBJECT

public:
    RemoveNode(const AbstractTreeModel& model, int node_id, QWidget* parent = nullptr);
    ~RemoveNode();

    void DisableRemove();

signals:
    // send to tree model
    void SRemoveMulti(int node_id);
    void SReplaceMulti(int old_node_id, int new_node_id);

private slots:
    void RCustomAccept();

private:
    Ui::RemoveNode* ui;
    void IniDialog();
    void IniConnect();

private:
    int node_id_ {};
    int section_ {};

    const AbstractTreeModel& model_;
};

#endif // REMOVENODE_H
