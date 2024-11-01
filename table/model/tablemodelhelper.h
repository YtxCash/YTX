#ifndef TABLEMODELHELPER_H
#define TABLEMODELHELPER_H

#include <QMutex>

#include "component/using.h"
#include "database/sqlite/sqlite.h"
#include "table/trans.h"

class TableModelHelper {
public:
    template <typename T>
    static bool UpdateField(Sqlite* sql, TransShadow* trans_shadow, CString& table, const T& value, CString& field, T* TransShadow::* member,
        const std::function<void()>& action = {})
    {
        if (trans_shadow == nullptr || trans_shadow->*member == nullptr || *(trans_shadow->*member) == value)
            return false;

        *(trans_shadow->*member) = value;

        if (*trans_shadow->rhs_node == 0)
            return false;

        sql->UpdateField(table, value, field, *trans_shadow->id);
        if (action)
            action();

        return true;
    }

    static void AccumulateSubtotal(QMutex& mutex, QList<TransShadow*>& trans_shadow_list, int start, bool rule);
    static double Balance(bool rule, double debit, double credit) { return (rule ? 1 : -1) * (credit - debit); };
    static bool UpdateRhsNode(TransShadow* trans_shadow, int value);
};

#endif // TABLEMODELHELPER_H
