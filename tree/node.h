#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QString>
#include <cmath>
#include <tuple>

struct Node {
    Node() = default;
    ~Node() = default;

    Node(const Node& other);
    Node& operator=(const Node& other);

    bool operator==(const Node& other) const noexcept;
    bool operator!=(const Node& other) const noexcept { return !(*this == other); }
    void Reset();

    Node(Node&&) noexcept = delete;
    Node& operator=(Node&&) noexcept = delete;

public:
    QString name {};
    int id {};
    QString code {};
    QString description {};
    QString note {};
    bool node_rule { false };
    bool branch { false };
    int unit {};

    int first {};
    double second {};
    double third {};
    bool fourth {};

    // order and stakeholder
    QString date_time {};
    int employee {};
    // order
    int party {};

    double final_total {};
    double initial_total {};

    Node* parent {};
    QList<Node*> children {};

private:
    static constexpr double tolerance = 1e-9;
};

inline Node::Node(const Node& other)
    : name(other.name)
    , id(other.id)
    , code(other.code)
    , description(other.description)
    , note(other.note)
    , node_rule(other.node_rule)
    , branch(other.branch)
    , unit(other.unit)
    , first(other.first)
    , second(other.second)
    , third(other.third)
    , fourth(other.fourth)
    , date_time(other.date_time)
    , employee(other.employee)
    , party(other.party)
    , final_total(other.final_total)
    , initial_total(other.initial_total)
{
    parent = nullptr;
    children.clear();
}

inline Node& Node::operator=(const Node& other)
{
    if (this == &other)
        return *this;

    name = other.name;
    id = other.id;
    code = other.code;
    description = other.description;
    note = other.note;
    node_rule = other.node_rule;
    branch = other.branch;
    unit = other.unit;
    first = other.first;
    second = other.second;
    third = other.third;
    fourth = other.fourth;
    date_time = other.date_time;
    employee = other.employee;
    party = other.party;
    final_total = other.final_total;
    initial_total = other.initial_total;

    parent = nullptr;
    children.clear();

    return *this;
}

inline bool Node::operator==(const Node& other) const noexcept
{
    return std::tie(name, id, code, party, employee, fourth, first, date_time, description, note, node_rule, branch, unit, parent, children)
        == std::tie(other.name, other.id, other.code, other.party, other.employee, other.fourth, other.first, other.date_time, other.description, other.note,
            other.node_rule, other.branch, other.unit, other.parent, other.children)
        && std::abs(second - other.second) < tolerance && std::abs(third - other.third) < tolerance;
}

inline void Node::Reset()
{
    name.clear();
    id = 0;
    code.clear();
    party = 0;
    employee = 0;
    second = 0.0;
    third = 0.0;
    fourth = false;
    first = 0;
    date_time.clear();
    description.clear();
    note.clear();
    node_rule = false;
    branch = false;
    unit = 0;
    final_total = 0.0;
    initial_total = 0.0;
    parent = nullptr;
    children.clear();
}

using NodeHash = QHash<int, Node*>;

#endif // NODE_H
