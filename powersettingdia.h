#ifndef POWERSETTINGDIA_H
#define POWERSETTINGDIA_H

#include <QDialog>

namespace Ui {
class PowerSettingDia;
}

struct ScreenSetting {
    ScreenSetting() = default;
    ScreenSetting(int on_brightness, int on_reflash, int off_brightness, int off_reflash)
    {
        brightness[0] = off_brightness;
        brightness[1] = on_brightness;
        reflash[0] = off_reflash;
        reflash[1] = on_reflash;
    }
    int brightness[2];
    int reflash[2];
};

class PowerSettingDia : public QDialog {
    Q_OBJECT

public:
    explicit PowerSettingDia(const ScreenSetting &screenSetting, QWidget *parent = nullptr);
    ~PowerSettingDia();

    static ScreenSetting readScreenSettings(void);

signals:
    void powerSettingApply(ScreenSetting screenSetting);

private:
    Ui::PowerSettingDia *ui;
};

#endif // POWERSETTINGDIA_H
