#include "preferences.h"

#include <QCompleter>
#include <QDir>
#include <QFileDialog>
#include <QTimer>

#include "component/constvalue.h"
#include "dialog/signalblocker.h"
#include "ui_preferences.h"

Preferences::Preferences(CInfo& info, const TreeModel* model, Interface interface, Settings settings, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::Preferences)
    , interface_ { interface }
    , settings_ { settings }
    , model_ { model }

{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    leaf_branch_model_ = new QStandardItemModel(this);
    model_->LeafPathBranchPathFPT(leaf_branch_model_);

    IniStringList();
    IniDialog(info.unit_hash);
    IniConnect();

    // 使用QTimer延迟执行
    QTimer::singleShot(100, this, [this, info]() { Data(); });
    DynamicLable(info.section);
}

Preferences::~Preferences() { delete ui; }

void Preferences::IniDialog(CStringHash& unit_hash)
{
    ui->listWidget->setCurrentRow(0);
    ui->stackedWidget->setCurrentIndex(0);
    ui->pBtnOk->setDefault(true);
    this->setWindowTitle(tr("Preferences"));

    IniCombo(ui->comboDateTime, date_format_list_);
    IniCombo(ui->comboLanguage, language_list_);
    IniCombo(ui->comboSeparator, separator_list_);
    IniCombo(ui->comboTheme, theme_list_);

    IniCombo(ui->comboDefaultUnit, unit_hash);

    ui->comboStatic->setModel(leaf_branch_model_);
    ui->comboDynamicLhs->setModel(leaf_branch_model_);
    ui->comboDynamicRhs->setModel(leaf_branch_model_);

    IniCombo(ui->comboOperation, operation_list_);
}

void Preferences::IniCombo(QComboBox* combo, CStringList& list)
{
    combo->addItems(list);
    combo->model()->sort(0);
}

void Preferences::IniCombo(QComboBox* combo, CStringHash& hash)
{
    for (auto it = hash.cbegin(); it != hash.cend(); ++it)
        combo->addItem(it.value(), it.key());

    combo->model()->sort(0);
}

void Preferences::IniConnect()
{
    connect(ui->listWidget, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(ui->pBtnOk, &QPushButton::clicked, this, &Preferences::on_pBtnApply_clicked);
}

void Preferences::Data()
{
    DataCombo(ui->comboTheme, interface_.theme);
    DataCombo(ui->comboLanguage, interface_.language);
    DataCombo(ui->comboSeparator, interface_.separator);
    DataCombo(ui->comboDateTime, interface_.date_format);

    DataCombo(ui->comboDefaultUnit, settings_.default_unit);
    ui->pBtnDocumentDir->setText(settings_.document_dir);
    ui->spinAmountDecimal->setValue(settings_.amount_decimal);
    ui->spinCommonDecimal->setValue(settings_.common_decimal);

    ui->lineStatic->setText(settings_.static_label);
    DataCombo(ui->comboStatic, settings_.static_node);
    ui->lineDynamic->setText(settings_.dynamic_label);
    DataCombo(ui->comboDynamicLhs, settings_.dynamic_node_lhs);
    DataCombo(ui->comboOperation, settings_.operation);
    DataCombo(ui->comboDynamicRhs, settings_.dynamic_node_rhs);

    ResizeLine(ui->lineStatic, settings_.static_label);
    ResizeLine(ui->lineDynamic, settings_.dynamic_label);
}

void Preferences::DataCombo(QComboBox* combo, int value)
{
    int item_index { combo->findData(value) };
    combo->setCurrentIndex(item_index);
}

void Preferences::DataCombo(QComboBox* combo, CString& string)
{
    int item_index { combo->findText(string) };
    combo->setCurrentIndex(item_index);
}

void Preferences::IniStringList()
{
    language_list_.emplaceBack(EN_US);
    language_list_.emplaceBack(ZH_CN);

    separator_list_.emplaceBack(DASH);
    separator_list_.emplaceBack(COLON);
    separator_list_.emplaceBack(SLASH);

    theme_list_.emplaceBack(SOLARIZED_DARK);

    operation_list_.emplaceBack(PLUS);
    operation_list_.emplaceBack(MINUS);

    date_format_list_.emplaceBack(DATE_TIME_FST);
    date_format_list_.emplaceBack(DATE_FST);
}

void Preferences::on_pBtnApply_clicked() { emit SUpdateSettings(settings_, interface_); }

void Preferences::on_pBtnDocumentDir_clicked()
{
    auto dir { ui->pBtnDocumentDir->text() };
    auto default_dir { QFileDialog::getExistingDirectory(this, tr("Select Directory"), QDir::homePath() + "/" + dir) };

    if (!default_dir.isEmpty()) {
        auto relative_path { QDir::home().relativeFilePath(default_dir) };
        settings_.document_dir = relative_path;
        ui->pBtnDocumentDir->setText(relative_path);
    }
}

void Preferences::on_pBtnResetDocumentDir_clicked()
{
    settings_.document_dir = QString();
    ui->pBtnDocumentDir->setText(QString());
}

void Preferences::ResizeLine(QLineEdit* line, CString& text) { line->setMinimumWidth(QFontMetrics(line->font()).horizontalAdvance(text) + 8); }

void Preferences::DynamicLable(Section section)
{
    switch (section) {
    case Section::kFinance:
        ui->labelAmountDecimal->setText(tr("Amount Decimal"));
        ui->labelCommonDecimal->setText(tr("FXRate Decimal"));
        break;
    case Section::kStakeholder:
        ui->labelAmountDecimal->setText(tr("Amount Decimal"));
        ui->labelCommonDecimal->setText(tr("Placeholder"));
        break;
    case Section::kTask:
    case Section::kProduct:
        ui->labelAmountDecimal->setText(tr("Amount Decimal"));
        ui->labelCommonDecimal->setText(tr("Quantity Decimal"));
        break;
    case Section::kSales:
    case Section::kPurchase:
        ui->labelAmountDecimal->setText(tr("Amount Decimal"));
        ui->labelCommonDecimal->setText(tr("Quantity Decimal"));
        break;
    default:
        break;
    }
}

void Preferences::on_comboDefaultUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    settings_.default_unit = ui->comboDefaultUnit->currentData().toInt();
}

void Preferences::on_comboStatic_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    settings_.static_node = ui->comboStatic->currentData().toInt();
}

void Preferences::on_comboDynamicLhs_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    settings_.dynamic_node_lhs = ui->comboDynamicLhs->currentData().toInt();
}

void Preferences::on_comboDynamicRhs_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    settings_.dynamic_node_rhs = ui->comboDynamicRhs->currentData().toInt();
}

void Preferences::on_spinAmountDecimal_editingFinished() { settings_.amount_decimal = ui->spinAmountDecimal->value(); }

void Preferences::on_lineStatic_editingFinished()
{
    settings_.static_label = ui->lineStatic->text();
    ResizeLine(ui->lineStatic, settings_.static_label);
}

void Preferences::on_lineDynamic_editingFinished()
{
    settings_.dynamic_label = ui->lineDynamic->text();
    ResizeLine(ui->lineDynamic, settings_.dynamic_label);
}

void Preferences::on_spinCommonDecimal_editingFinished() { settings_.common_decimal = ui->spinCommonDecimal->value(); }

void Preferences::on_comboTheme_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    interface_.theme = ui->comboTheme->currentText();
}

void Preferences::on_comboLanguage_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    interface_.language = ui->comboLanguage->currentText();
}

void Preferences::on_comboDateTime_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    interface_.date_format = ui->comboDateTime->currentText();
}

void Preferences::on_comboSeparator_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    interface_.separator = ui->comboSeparator->currentText();
}

void Preferences::on_comboOperation_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    settings_.operation = ui->comboOperation->currentText();
}
