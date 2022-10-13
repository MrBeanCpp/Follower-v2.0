#include "inputmethod.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <psapi.h>
InputMethod::InputMethod(const QString& path)
    : listPath(path)
{
    hkl = LoadKeyboardLayoutA("0x0409", KLF_ACTIVATE); //EN
    readListFile();
}

void InputMethod::checkAndSetEn()
{
    HWND hwnd = GetForegroundWindow();
    if (lastHwnd == hwnd) return;
    QString title = getWindowTitle(hwnd);
    QString processName = getProcessNameByPID(getForeWindowPID(hwnd));
    //qDebug() << title << processName;
    if (isWindowTitleInList(title) || isProcessNameInList(processName))
        setEnMode(hwnd);
    lastHwnd = hwnd;
}

bool InputMethod::isInList(const QString& title)
{
    for (const auto& str : qAsConst(list))
        if (title.contains(str, Qt::CaseInsensitive))
            return true;
    return false;
}

bool InputMethod::isWindowTitleInList(const QString& title)
{
    for (const QString& str : qAsConst(list))
        if (!str.endsWith(".exe") && title.contains(str, Qt::CaseInsensitive)) {
            //qDebug() << "匹配：" << str << " in " << title;
            return true;
        }
    return false;
}

bool InputMethod::isProcessNameInList(const QString& name)
{
    for (const QString& str : qAsConst(list))
        if (name == str) {
            //qDebug() << "匹配：" << str << " == " << name;
            return true;
        }
    return false;
}

DWORD InputMethod::getForeWindowPID(HWND hwnd)
{
    if (hwnd == NULL)
        hwnd = GetForegroundWindow();
    DWORD processId = -1; //not NULL
    GetWindowThreadProcessId(hwnd, &processId);
    return processId;
}

QString InputMethod::getProcessNameByPID(DWORD PID)
{
    static WCHAR path[512];
    HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    GetProcessImageFileNameW(Process, path, _countof(path)); //not sizeof
    CloseHandle(Process);

    QString pathS = QString::fromWCharArray(path);
    return pathS.mid(pathS.lastIndexOf('\\') + 1);
}

QString InputMethod::getWindowTitle(HWND hwnd)
{
    static WCHAR title[256]; //确保都用宽字符 Unicode
    if (hwnd == NULL)
        hwnd = GetForegroundWindow();
    GetWindowTextW(hwnd, title, _countof(title)); //not sizeof
    return QString::fromWCharArray(title); //解决中文乱码
}

void InputMethod::setEnMode(HWND hwnd)
{
    PostMessageA(hwnd, WM_INPUTLANGCHANGEREQUEST, (WPARAM)TRUE, (LPARAM)hkl);
    //qDebug() << "#set EN++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";
}

void InputMethod::readListFile()
{
    QFile file(listPath);
    if (file.open(QFile::ReadWrite | QFile::Text)) {
        list.clear(); //Don't Forget [2022.1.25]
        QTextStream text(&file);
        text.setCodec("UTF-8");
        while (!text.atEnd())
            list << text.readLine();
        file.close();
    }
}
