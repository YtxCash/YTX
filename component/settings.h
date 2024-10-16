#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>

// Struct representing interface settings
struct Interface {
    QString theme {};
    QString language {};
    QString date_format {};
    QString separator {};

    // Equality operator overload to compare two Interface structs
    bool operator==(const Interface& other) const noexcept
    {
        return std::tie(theme, language, date_format, separator) == std::tie(other.theme, other.language, other.date_format, other.separator);
    }

    // Inequality operator overload to compare two Interface structs
    bool operator!=(const Interface& other) const noexcept { return !(*this == other); }
};

// Struct representing section settings
struct Settings {
    QString static_label {};
    int static_node {};
    QString dynamic_label {};
    int dynamic_node_lhs {};
    QString operation {};
    int dynamic_node_rhs {};
    int default_unit {};
    QString document_dir {};
    int amount_decimal {};
    int common_decimal {};

    // Equality operator overload to compare two Settings structs
    bool operator==(const Settings& other) const noexcept
    {
        return std::tie(static_label, static_node, dynamic_label, dynamic_node_lhs, operation, dynamic_node_rhs, default_unit, document_dir, amount_decimal,
                   common_decimal)
            == std::tie(other.static_label, other.static_node, other.dynamic_label, other.dynamic_node_lhs, other.operation, other.dynamic_node_rhs,
                other.default_unit, other.document_dir, other.amount_decimal, other.common_decimal);
    }

    // Inequality operator overload to compare two Settings structs
    bool operator!=(const Settings& other) const noexcept { return !(*this == other); }
};

using CInterface = const Interface;
using CSettings = const Settings;

#endif // SETTINGS_H
