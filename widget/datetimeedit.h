#ifndef DATETIMEEDIT_H
#define DATETIMEEDIT_H

#include <QDateTimeEdit>

class DateTimeEdit final : public QDateTimeEdit {
    Q_OBJECT

public:
    DateTimeEdit(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    bool LastMonthEnd(QDateTime& date_time);
    bool NextMonthStart(QDateTime& date_time);
    bool HandleSpecialKeys(int key, QDateTime& date_time);
    bool AdjustDateTime(QDateTime& date_time, int days = 0, int months = 0, int years = 0);
    bool SetToCurrentDateTime(QDateTime& date_time);
};

#endif // DATETIMEEDIT_H
