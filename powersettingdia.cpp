#include "powersettingdia.h"
#include "ui_powersettingdia.h"
#include "systemapi.h"
#include <QSettings>
#include "path.h"
PowerSettingDia::PowerSettingDia(int on_brightness, int on_reflash, int off_brightness, int off_reflash, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PowerSettingDia)
{
    ui->setupUi(this);
    setWindowTitle("PowerSetting");
    resize(DPI(size()));

    ui->on_brightness->setValue(on_brightness);
    ui->on_reflash->setValue(on_reflash);
    ui->off_brightness->setValue(off_brightness);
    ui->off_reflash->setValue(off_reflash);

    connect(ui->pushButton, &QPushButton::clicked, this, [=](){
        emit powerSettingApply(ui->on_brightness->value(), ui->on_reflash->value(), ui->off_brightness->value(), ui->off_reflash->value());
    });

    connect(this, &PowerSettingDia::powerSettingApply, this, [=](int on_brightness, int on_reflash, int off_brightness, int off_reflash){
        QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
        IniSet.setValue("PowerSetting/PowerOn/brightness", on_brightness);
        IniSet.setValue("PowerSetting/PowerOn/reflash rate", on_reflash);
        IniSet.setValue("PowerSetting/PowerOff/brightness", off_brightness);
        IniSet.setValue("PowerSetting/PowerOff/reflash rate", off_reflash);
    });
}

PowerSettingDia::~PowerSettingDia()
{
    delete ui;
}
