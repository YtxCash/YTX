#include "mainwindowutils.h"

QVariantList MainWindowUtils::SaveTab(CTableHash& table_hash)
{
    if (table_hash.isEmpty())
        return {};

    const auto keys { table_hash.keys() };
    QVariantList list {};

    for (int node_id : keys)
        list.emplaceBack(node_id);

    return list;
}

QSet<int> MainWindowUtils::ReadSettings(std::shared_ptr<QSettings> settings, CString& section, CString& property)
{
    if (!settings)
        return {};

    auto variant { settings->value(QString("%1/%2").arg(section, property)) };

    if (!variant.isValid() || !variant.canConvert<QVariantList>())
        return {};

    QSet<int> set {};
    const auto variant_list { variant.value<QVariantList>() };

    for (const auto& node_id : variant_list)
        set.insert(node_id.toInt());

    return set;
}

void MainWindowUtils::WriteSettings(std::shared_ptr<QSettings> settings, const QVariant& value, CString& section, CString& property)
{
    if (!settings) {
        qWarning() << "WriteTabID: Invalid parameters (settings is null)";
        return;
    }

    settings->setValue(QString("%1/%2").arg(section, property), value);
}
