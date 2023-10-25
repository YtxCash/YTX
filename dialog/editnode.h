#ifndef EDITNODE_H
#define EDITNODE_H

#include "../tree/node.h"
#include <QDialog>

namespace Ui {
class EditNode;
}

class EditNode : public QDialog {
    Q_OBJECT

public:
    EditNode(Node* node, bool usage, QWidget* parent = nullptr);
    ~EditNode();

signals:
    void SendUpdate(const Node* node);

private slots:
    void CustomAccept();

private:
    void IniDialog();
    void IniConnect();
    void Data(Node* node);
    void SetData();

private:
    Ui::EditNode* ui;
    Node* node_ {};
    bool usage_ {};
};

#endif // EDITNODE_H
