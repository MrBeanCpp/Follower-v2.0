#include "systemapi.h"
#include "widget.h"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QSharedMemory>
#include <QTimer>
#include <QScreen>
#include "logfilehandler.h"
void init(void);
void checkAutoStart(void);
void checkSingleApp(void);
int main(int argc, char* argv[])
{
    //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    LogFileHandler::startFileLogging();
    SystemAPI sysAPI; //用以初始化sys全局指针,like qApp
    init();
    Widget w;
    w.show();
    return a.exec();
}
void init(void)
{
    checkSingleApp(); //检测重复启动
    //checkAutoStart(); //检测自启动
}
void checkAutoStart()
{
    QString appName = "Follower v2.0"; //QApplication::applicationName() //程序名称
    QString appPath = QApplication::applicationFilePath(); // 程序路径
    appPath = appPath.replace("/", "\\");
    QSettings reg("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    QString val = reg.value(appName).toString(); //如果此键不存在，则返回的是空字符串
    if (val != appPath)
        reg.setValue(appName, appPath); //如果移除的话，reg->remove(applicationName);
}
void checkSingleApp()
{
    QSharedMemory* singleIns = new QSharedMemory("Follower v2.0 -MrBeanC SingleLock", qApp); //唯一Key
    if (!singleIns->create(1)) { //检测是否存在
        QMessageBox::warning(nullptr, "Error", "Follower v2.0 has been started!");
        //delete qApp; //qApp->quit()失效，因为未进入事件循环
        //delete singleIns;
        //exit(-1);
        QTimer::singleShot(0, [=]() { qApp->quit(); }); //等待进入事件循环
    }
}
