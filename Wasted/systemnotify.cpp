#include "systemnotify.h"
#include <QDebug>
#include <windows.h>

NOTIFYICONDATA SystemNotify::tipData; //保证唯一性

SystemNotify::SystemNotify()
{
    tipData.cbSize = NOTIFYICONDATA_V2_SIZE; // 结构大小 不是sizeof！！！！！！！！！存在变长数组
    tipData.uVersion = NOTIFYICON_VERSION;
}

SystemNotify::SystemNotify(HWND hwnd)
    : SystemNotify()
{
    SetupTrayIcon(hwnd);
}

bool SystemNotify::SetupTrayIcon(HWND hwnd)
{
    if (tipData.hWnd != NULL) return true;
    tipData.hWnd = hwnd; // 接收 托盘通知消息 的窗口句柄
    //tipData.hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(0));
    //tipData.uID = 0;
    tipData.uFlags = NIF_GUID; //保留
    return Shell_NotifyIcon(NIM_ADD, &tipData);
}

bool SystemNotify::ShowBalloonTip(QString szMsg, QString szTitle, UINT uTimeout)
{
    if (tipData.hWnd == NULL) return false;
    tipData.uFlags = NIF_INFO; //information
    tipData.uTimeout = uTimeout;
    tipData.dwInfoFlags = NIIF_INFO;
    wcscpy(tipData.szInfo, szMsg.toStdWString().c_str());
    wcscpy(tipData.szInfoTitle, szTitle.toStdWString().c_str());
    if (Shell_NotifyIcon(NIM_MODIFY, &tipData))
        return true;
    else {
        tipData.uFlags = NIF_GUID; //保留
        Shell_NotifyIcon(NIM_ADD, &tipData); //任务栏重启后，尝试重新建立托盘//uFlags = NIF_GUID;!!!!
        tipData.uFlags = NIF_INFO;
        return Shell_NotifyIcon(NIM_MODIFY, &tipData);
    }
}

SystemNotify::~SystemNotify()
{
    Shell_NotifyIcon(NIM_DELETE, &tipData);
}
