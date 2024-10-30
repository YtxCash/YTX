#ifndef NODE_H
#define NODE_H

#include <QList>
#include <QString>
#include <cmath>
#include <tuple>

constexpr double TOLERANCE = 1e-9;

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

    QString name {};
    int id {};
    QString code {};
    QString description {};
    QString note {};
    bool rule { false };
    bool branch { false };
    int unit {};

    double first {};
    double second {};
    double discount {};
    bool locked {};

    // order and stakeholder
    QString date_time {};
    int employee {};
    // order
    int party {};

    double final_total {};
    double initial_total {};

    Node* parent {};
    QList<Node*> children {};
};

inline Node::Node(const Node& other)
    : name(other.name)
    , id(other.id)
    , code(other.code)
    , description(other.description)
    , note(other.note)
    , rule(other.rule)
    , branch(other.branch)
    , unit(other.unit)
    , first(other.first)
    , second(other.second)
    , discount(other.discount)
    , locked(other.locked)
    , date_time(other.date_time)
    , employee(other.employee)
    , party(other.party)
    , final_total(other.final_total)
    , initial_total(other.initial_total)
{
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
    rule = other.rule;
    branch = other.branch;
    unit = other.unit;
    first = other.first;
    second = other.second;
    discount = other.discount;
    locked = other.locked;
    date_time = other.date_time;
    employee = other.employee;
    party = other.party;
    final_total = other.final_total;
    initial_total = other.initial_total;

    return *this;
}

inline bool Node::operator==(const Node& other) const noexcept
{
    return std::tie(name, id, code, party, employee, locked, first, date_time, description, note, rule, branch, unit, parent, children)
        == std::tie(other.name, other.id, other.code, other.party, other.employee, other.locked, other.first, other.date_time, other.description, other.note,
            other.rule, other.branch, other.unit, other.parent, other.children)
        && std::abs(second - other.second) < TOLERANCE && std::abs(discount - other.discount) < TOLERANCE;
}

inline void Node::Reset()
{
    name.clear();
    id = 0;
    code.clear();
    party = 0;
    employee = 0;
    second = 0.0;
    discount = 0.0;
    locked = false;
    first = 0.0;
    date_time.clear();
    description.clear();
    note.clear();
    rule = false;
    branch = false;
    unit = 0;
    final_total = 0.0;
    initial_total = 0.0;
    parent = nullptr;
    children.clear();
}

struct NodeShadow {
    void Reset();
    void Set(Node* node);

    QString* name {};
    int* id {};
    QString* code {};
    QString* description {};
    QString* note {};
    bool* rule {};
    bool* branch {};
    int* unit {};

    double* first {};
    double* second {};
    double* discount {};
    bool* locked {};

    QString* date_time {};
    int* employee {};
    int* party {};

    double* final_total {};
    double* initial_total {};
};

inline void NodeShadow::Reset()
{
    name = nullptr;
    id = nullptr;
    code = nullptr;
    description = nullptr;
    note = nullptr;
    rule = nullptr;
    branch = nullptr;
    unit = nullptr;

    first = nullptr;
    second = nullptr;
    discount = nullptr;
    locked = nullptr;

    date_time = nullptr;
    employee = nullptr;
    party = nullptr;

    final_total = nullptr;
    initial_total = nullptr;
}

inline void NodeShadow::Set(Node* node)
{
    if (node) {
        name = &node->name;
        id = &node->id;
        code = &node->code;
        description = &node->description;
        note = &node->note;
        rule = &node->rule;
        branch = &node->branch;
        unit = &node->unit;

        first = &node->first;
        second = &node->second;
        discount = &node->discount;
        locked = &node->locked;

        date_time = &node->date_time;
        employee = &node->employee;
        party = &node->party;

        final_total = &node->final_total;
        initial_total = &node->initial_total;
    }
}

using NodeHash = QHash<int, Node*>;
using CNodeHash = const QHash<int, Node*>;

#endif // NODE_H
