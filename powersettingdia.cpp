#include "powersettingdia.h"
#include "ui_powersettingdia.h"
#include "systemapi.h"
#include <QSettings>
#include "path.h"
#include <QToolTip>
#include "WinUtility.h"
#include <QSet>
PowerSettingDia::PowerSettingDia(const ScreenSetting &screenSetting, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PowerSettingDia)
{
    ui->setupUi(this);
    setWindowTitle("PowerSetting [-1 == unused]");
    resize(DPI(size()));

    QSet<DWORD> reflashList = Win::getAvailableScreenReflashRates();
    ui->on_reflash->addItem("unused", -1);
    ui->off_reflash->addItem("unused", -1);
    for (DWORD reflash : reflashList) {
        ui->on_reflash->addItem(QString::number(reflash) + "Hz", (int)reflash);
        ui->off_reflash->addItem(QString::number(reflash) + "Hz", (int)reflash);
    }
    int index_on = ui->on_reflash->findData(screenSetting.reflash[1]);
    int index_off = ui->off_reflash->findData(screenSetting.reflash[0]);
    ui->on_reflash->setCurrentIndex(qMax(index_on, 0)); //not found == -1
    ui->off_reflash->setCurrentIndex(qMax(index_off, 0));

    connect(ui->pushButton, &QPushButton::clicked, this, [=]() {
        emit powerSettingApply(
            ScreenSetting(ui->on_brightness->value(),
                          ui->on_reflash->currentData().toInt(),
                          ui->off_brightness->value(),
                          ui->off_reflash->currentData().toInt()));
    });

    connect(this, &PowerSettingDia::powerSettingApply, this, [=](ScreenSetting screenSetting) {
        QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
        IniSet.setValue("PowerSetting/PowerOn/brightness", screenSetting.brightness[1]);
        IniSet.setValue("PowerSetting/PowerOn/reflash rate", screenSetting.reflash[1]);
        IniSet.setValue("PowerSetting/PowerOff/brightness", screenSetting.brightness[0]);
        IniSet.setValue("PowerSetting/PowerOff/reflash rate", screenSetting.reflash[0]);
    });

    connect(ui->on_brightness, &QSlider::valueChanged, this, [=](int value) {
        QString str = QString::number(value);
        if (value == -1) str = "-1:unused";
        QToolTip::showText(QPoint(QCursor::pos().x(), ui->on_brightness->mapToGlobal(QPoint(0, 0)).y() - DPI(35)), str);
        ui->on_brightness->setToolTip(str);
    });
    connect(ui->off_brightness, &QSlider::valueChanged, this, [=](int value) {
        QString str = QString::number(value);
        if (value == -1) str = "-1:unused";
        QToolTip::showText(QPoint(QCursor::pos().x(), ui->off_brightness->mapToGlobal(QPoint(0, 0)).y() - DPI(35)), str);
        ui->off_brightness->setToolTip(str);
    });

    ui->on_brightness->setValue(screenSetting.brightness[1]);
    //ui->on_reflash->setValue(screenSetting.reflash[1]);
    ui->off_brightness->setValue(screenSetting.brightness[0]);
    //ui->off_reflash->setValue(screenSetting.reflash[0]);
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
