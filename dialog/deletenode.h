#ifndef DELETENODE_H
#define DELETENODE_H

#include <QDialog>

namespace Ui {
class DeleteNode;
}

class DeleteNode : public QDialog {
    Q_OBJECT

public:
    DeleteNode(int node_id, const QMultiMap<QString, int>& leaf_map, QWidget* parent = nullptr);
    ~DeleteNode();

signals:
    void SendDelete(int node_id);
    void SendReplace(int old_node_id, int new_node_id);
    void SendReloadAll(int node_id);

private slots:
    void CustomAccept();

private:
    Ui::DeleteNode* ui;
    void IniDialog();
    void IniConnect();

private:
    int node_id_ {};
};

#endif // DELETENODE_H
