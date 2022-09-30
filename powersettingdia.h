#ifndef POWERSETTINGDIA_H
#define POWERSETTINGDIA_H

#include <QDialog>

namespace Ui {
class PowerSettingDia;
}

class PowerSettingDia : public QDialog
{
    Q_OBJECT

public:
    explicit PowerSettingDia(int on_brightness, int on_reflash, int off_brightness, int off_reflash, QWidget *parent = nullptr);
    ~PowerSettingDia();

signals:
    void powerSettingApply(int on_brightness, int on_reflash, int off_brightness, int off_reflash);

private:
    Ui::PowerSettingDia *ui;
};

#endif // POWERSETTINGDIA_H
