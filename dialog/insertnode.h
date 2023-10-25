#ifndef INSERTNODE_H
#define INSERTNODE_H

#include "../tree/node.h"
#include <QDialog>

namespace Ui {
class InsertNode;
}

class InsertNode : public QDialog {
    Q_OBJECT

public:
    InsertNode(Node* node, QWidget* parent = nullptr);
    ~InsertNode();

private slots:
    void CustomAccept();
    void SetWindowTitle(const QString& name);

private:
    void IniDialog();
    void IniConnect();
    void SetData();

private:
    Ui::InsertNode* ui;
    Node* node_ {};
};

#endif // INSERTNODE_H
