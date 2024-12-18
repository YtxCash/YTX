#ifndef DATA_H
#define DATA_H

#include "component/info.h"
#include "component/tab.h"
#include "database/sqlite/sqlite.h"

struct Data {
    Tab tab {};
    Info info {};
    Sqlite* sql {};
};

using CData = const Data;

#endif // DATA_H
