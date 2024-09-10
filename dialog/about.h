#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

namespace Ui {
class About;
}

class About final : public QDialog {
    Q_OBJECT

public:
    explicit About(QWidget* parent = nullptr);
    ~About();

private slots:
    void on_pBtnLink_clicked();

private:
    void IniDialog();

private:
    Ui::About* ui;
};

#endif // ABOUT_H
