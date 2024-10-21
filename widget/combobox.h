#ifndef COMBOBOX_H
#define COMBOBOX_H

#include <QApplication>
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

        auto* completer { new QCompleter(model(), this) };
        completer->setFilterMode(Qt::MatchContains);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        setCompleter(completer);
        setSizeAdjustPolicy(QComboBox::AdjustToContents);
    }

    QSize sizeHint() const override
    {
        QSize sz = QComboBox::sizeHint();

        int scroll_bar_width = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        sz.setWidth(sz.width() + scroll_bar_width);

        return sz;
    }
};

#endif // COMBOBOX_H
