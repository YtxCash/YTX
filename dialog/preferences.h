#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QComboBox>
#include <QDialog>

#include "component/enumclass.h"
#include "component/info.h"
#include "component/settings.h"
#include "component/using.h"
#include "tree/model/treemodel.h"

namespace Ui {
class Preferences;
}

class Preferences final : public QDialog {
    Q_OBJECT

public:
    Preferences(CInfo& info, const TreeModel* model, CStringList& date_format_list, Interface interface, Settings settings, QWidget* parent = nullptr);
    ~Preferences();

signals:
    void SUpdateSettings(CSettings& settings, CInterface& interface);

private slots:
    void on_pBtnApply_clicked();
    void on_pBtnDocumentDir_clicked();
    void on_pBtnResetDocumentDir_clicked();

    void on_comboDefaultUnit_currentIndexChanged(int index);

    void on_comboOperation_currentIndexChanged(int index);
    void on_comboStatic_currentIndexChanged(int index);
    void on_comboDynamicLhs_currentIndexChanged(int index);
    void on_comboDynamicRhs_currentIndexChanged(int index);

    void on_spinAmountDecimal_editingFinished();
    void on_spinCommonDecimal_editingFinished();
    void on_lineStatic_editingFinished();
    void on_lineDynamic_editingFinished();

    void on_comboTheme_currentIndexChanged(int index);
    void on_comboLanguage_currentIndexChanged(int index);
    void on_comboDateTime_currentIndexChanged(int index);
    void on_comboSeparator_currentIndexChanged(int index);
    void on_checkHideTime_toggled(bool checked);

private:
    void IniDialog(CStringHash& unit_hash, CStringList& date_format_list);
    void IniCombo(QComboBox* combo, const TreeModel* model);
    void IniCombo(QComboBox* combo, CStringList& list);
    void IniCombo(QComboBox* combo, CStringHash& hash);

    void IniConnect();
    void IniStringList();
    void ResizeLine(QLineEdit* line, CString& text);
    void DynamicLable(Section section);

    void Data();
    void DataCombo(QComboBox* combo, int value);
    void DataCombo(QComboBox* combo, CString& string);

private:
    Ui::Preferences* ui;

    QStringList theme_list_ {};
    QStringList language_list_ {};
    QStringList separator_list_ {};
    QStringList operation_list_ {};

    Interface interface_ {};
    Settings settings_ {};
    const TreeModel* model_ {};
};

#endif // PREFERENCES_H
