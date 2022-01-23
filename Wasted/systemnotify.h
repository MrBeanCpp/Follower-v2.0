#ifndef SYSTEMNOTIFY_H
#define SYSTEMNOTIFY_H

#include <QApplication>
#include <QString>
#include <windows.h>
class SystemNotify
{
public:
    SystemNotify();
    SystemNotify(HWND hwnd);

    bool SetupTrayIcon(HWND hwnd);
    bool ShowBalloonTip(QString szMsg, QString szTitle, UINT uTimeout = 1000);

    ~SystemNotify();

private:
    static NOTIFYICONDATA tipData;
};

#endif // SYSTEMNOTIFY_H
