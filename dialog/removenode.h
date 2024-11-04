#ifndef REMOVENODE_H
#define REMOVENODE_H

#include <QDialog>

#include "tree/model/treemodel.h"

namespace Ui {
class RemoveNode;
}

class RemoveNode final : public QDialog {
    Q_OBJECT

public:
    RemoveNode(CTreeModel* model, int node_id, int unit, bool disable, QWidget* parent = nullptr);
    ~RemoveNode();

signals:
    // send to tree model
    void SRemoveNode(int node_id);
    void SReplaceNode(int old_node_id, int new_node_id);

private slots:
    void RCustomAccept();

private:
    Ui::RemoveNode* ui;
    void IniDialog();
    void IniConnect();

    void DisableRemove();

private:
    int node_id_ {};
    int unit_ {};
    int section_ {};

    CTreeModel* model_ {};
};

#endif // REMOVENODE_H
