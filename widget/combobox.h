#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QComboBox>
#include <QCompleter>

class ComboBox final : public QComboBox {
    Q_OBJECT

public:
    explicit ComboBox(QWidget* parent = nullptr)
        : QComboBox { parent }
    {
        setFrame(false);
        setEditable(true);
        setInsertPolicy(QComboBox::NoInsert);

        auto completer { new QCompleter(model(), this) };
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        setCompleter(completer);
        setSizeAdjustPolicy(QComboBox::AdjustToContents);
    }
};

#endif // COMBOBOX_H
