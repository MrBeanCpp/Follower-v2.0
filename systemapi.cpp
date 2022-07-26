#include "systemapi.h"
#include "TableEditor/cmdeditor.h"
#include "TableEditor/noteeditor.h"
#include "path.h"
#include <QAction>
#include <QDebug>
#include <QDir>
#include <QMenu>
#include <QTimer>
#include <QScreen>
SystemAPI* sys = nullptr;

QSystemTrayIcon* SystemAPI::sysTray = nullptr;
InputMethod* SystemAPI::inputM = nullptr;
GapTimer* SystemAPI::gTimer = nullptr;
CmdEditor* SystemAPI::cmdEditor = nullptr;
NoteEditor* SystemAPI::noteEditor = nullptr;
qreal SystemAPI::scaleRatio = 1.0;

SystemAPI::SystemAPI(QObject* parent)
    : QObject(parent)
{ //need a function to delete these pointers(InputMethod is not a QObject but it Can be)//或者采用shared_ptr调用SystemAPI析构
    if (sys != nullptr) {
        qDebug() << "SystemAPI multi defined"; //只允许有一个对象
        QTimer::singleShot(0, [=]() { qApp->quit(); }); //等待进入事件循环
        return;
    }
    sys = this;

    Init();
    Init_SystemTray();
    Init_InputMethod();
    Init_GapTimer();
    Init_CmdEditor();
    Init_NoteEditor();
}

SystemAPI::~SystemAPI()
{
    if (inputM == nullptr) return;
    delete inputM;
    inputM = nullptr;
}

void SystemAPI::Init()
{
    QDir().mkdir(Path::dirPath()); //Core-Info
    scaleRatio = qApp->primaryScreen()->logicalDotsPerInch() / 96; //high DPI scale
    qDebug() << scaleRatio << qApp->primaryScreen()->physicalDotsPerInch() << qApp->primaryScreen()->size();
}

void SystemAPI::Init_SystemTray()
{
    if (sysTray != nullptr) return;
    sysTray = new QSystemTrayIcon(QIcon(":/ICON.ico"), qApp);
}

void SystemAPI::Init_InputMethod()
{
    if (inputM != nullptr) return;
    inputM = new InputMethod(Path::inputMethodList()); //no qApp
}

void SystemAPI::Init_GapTimer()
{
    if (gTimer != nullptr) return;
    gTimer = new GapTimer(qApp);
    gTimer->start(1000);
}

void SystemAPI::Init_CmdEditor()
{
    if (cmdEditor != nullptr) return;
    cmdEditor = new CmdEditor(Path::cmdList());
}

void SystemAPI::Init_NoteEditor()
{
    if (noteEditor != nullptr) return;
    noteEditor = new NoteEditor(Path::noteList());
}
