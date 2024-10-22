#ifndef DATEEDIT_H
#define DATEEDIT_H

#include <QDateEdit>

class DateEdit final : public QDateEdit {
    Q_OBJECT

public:
    DateEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool LastMonthEnd(QDate& date);
    bool NextMonthStart(QDate& date);

    bool HandleSpecialKeys(int key, QDate& date);
    bool AdjustDate(QDate& date, int days = 0, int months = 0, int years = 0);
    bool SetToCurrentDate(QDate& date);
};

#endif // DATEEDIT_H
