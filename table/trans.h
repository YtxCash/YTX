#ifndef TRANS_H
#define TRANS_H

#include <QSharedPointer>
#include <QStringList>

struct Trans {
    int id {};
    QString date_time {};
    QString code {};
    int lhs_node {};
    double lhs_ratio { 1.0 };
    double lhs_debit {};
    double lhs_credit {};
    QString description {};
    QStringList document {};
    bool state { false };
    double rhs_credit {};
    double rhs_debit {};
    double rhs_ratio { 1.0 };
    int rhs_node {};

    // order
    int node_id {};
    double discount_price {};
    double unit_price {};

    void Reset()
    {
        id = 0;
        date_time.clear();
        code.clear();
        lhs_node = 0;
        lhs_ratio = 1.0;
        lhs_debit = 0.0;
        lhs_credit = 0.0;
        description.clear();
        rhs_node = 0;
        rhs_ratio = 1.0;
        rhs_debit = 0.0;
        rhs_credit = 0.0;
        state = false;
        document.clear();
        node_id = 0;
        discount_price = 0.0;
        unit_price = 0.0;
    }
};

struct TransShadow {
    int* id {};
    QString* date_time {};
    QString* code {};
    int* lhs_node {};
    double* lhs_ratio {};
    double* lhs_debit {};
    double* lhs_credit {};
    QString* description {};
    QStringList* document {};
    bool* state {};
    double* rhs_credit {};
    double* rhs_debit {};
    double* rhs_ratio {};
    int* rhs_node {};
    int* node_id {};
    double* discount_price {};
    double* unit_price {};

    double subtotal {};

    void Reset()
    {
        id = nullptr;
        date_time = nullptr;
        code = nullptr;
        lhs_node = nullptr;
        lhs_ratio = nullptr;
        lhs_debit = nullptr;
        lhs_credit = nullptr;
        description = nullptr;
        rhs_node = nullptr;
        rhs_ratio = nullptr;
        rhs_debit = nullptr;
        rhs_credit = nullptr;
        state = nullptr;
        document = nullptr;
        node_id = nullptr;
        discount_price = nullptr;
        unit_price = nullptr;

        subtotal = 0.0;
    }
};

using TransList = QList<Trans*>;
using TransShadowList = QList<TransShadow*>;

#endif // TRANS_H
