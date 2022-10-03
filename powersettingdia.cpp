#include "powersettingdia.h"
#include "ui_powersettingdia.h"
#include "systemapi.h"
#include <QSettings>
#include "path.h"
PowerSettingDia::PowerSettingDia(const ScreenSetting &screenSetting, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PowerSettingDia)
{
    ui->setupUi(this);
    setWindowTitle("PowerSetting [-1 == unused]");
    resize(DPI(size()));

    ui->on_brightness->setValue(screenSetting.brightness[1]);
    ui->on_reflash->setValue(screenSetting.reflash[1]);
    ui->off_brightness->setValue(screenSetting.brightness[0]);
    ui->off_reflash->setValue(screenSetting.reflash[0]);

    connect(ui->pushButton, &QPushButton::clicked, this, [=]() {
        emit powerSettingApply(ScreenSetting(ui->on_brightness->value(), ui->on_reflash->value(), ui->off_brightness->value(), ui->off_reflash->value()));
    });

    connect(this, &PowerSettingDia::powerSettingApply, this, [=](ScreenSetting screenSetting) {
        QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
        IniSet.setValue("PowerSetting/PowerOn/brightness", screenSetting.brightness[1]);
        IniSet.setValue("PowerSetting/PowerOn/reflash rate", screenSetting.reflash[1]);
        IniSet.setValue("PowerSetting/PowerOff/brightness", screenSetting.brightness[0]);
        IniSet.setValue("PowerSetting/PowerOff/reflash rate", screenSetting.reflash[0]);
    });
}

PowerSettingDia::~PowerSettingDia()
{
    delete ui;
}

ScreenSetting PowerSettingDia::readScreenSettings()
{
    QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
    ScreenSetting screenSetting;
    screenSetting.brightness[1] = IniSet.value("PowerSetting/PowerOn/brightness", -1).toInt();
    screenSetting.reflash[1] = IniSet.value("PowerSetting/PowerOn/reflash rate", -1).toInt();
    screenSetting.brightness[0] = IniSet.value("PowerSetting/PowerOff/brightness", -1).toInt();
    screenSetting.reflash[0] = IniSet.value("PowerSetting/PowerOff/reflash rate", -1).toInt();
    return screenSetting;
}
