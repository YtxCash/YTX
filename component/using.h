#ifndef USING_H
#define USING_H

#include <QHash>
#include <QStringList>

using CStringHash = const QHash<int, QString>;
using StringHash = QHash<int, QString>;

using CString = const QString;
using CVariant = const QVariant;
using CStringList = const QStringList;

#endif // USING_H
