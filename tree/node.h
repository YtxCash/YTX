#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Tree {
enum Column {
    kName,
    kID,
    kDescription,
    kRule,
    kPlaceholder,
    kTotal
};
}
QT_END_NAMESPACE

struct Node {
public:
    Node() = default;
    ~Node();
    Node(int id, const QString& name, const QString& description, double total, bool rule, bool placeholder);
    void ResetToDefault();

public:
    int id { 0 };
    QString name {};
    QString description {};
    bool rule { false };
    bool placeholder { false };
    double total { 0.0 };

    Node* parent { nullptr };
    QList<Node*> children {};

private:
    Node(const Node& other) = delete;
    Node& operator=(const Node& other) = delete;
};

#endif // NODE_H
