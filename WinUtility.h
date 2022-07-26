#ifndef WIN_H
#define WIN_H
#include <QPoint>
#include <QString>
#include <windows.h>
#include <QRect>
class Win //Windows API
{
public:
    Win() = delete;
    static void setAlwaysTop(HWND hwnd, bool isTop = true);
    static void jumpToTop(HWND hwnd);
    static bool isInSameThread(HWND hwnd_1, HWND hwnd_2);
    static bool isTopMost(HWND hwnd);
    static QString getWindowText(HWND hwnd);
    static void miniAndShow(HWND hwnd); //最小化然后弹出以获取焦点
    static DWORD getProcessID(HWND hwnd);
    static QString getProcessName(HWND hwnd);
    static void getInputFocus(HWND hwnd);
    static void simulateKeyEvent(const QList<BYTE>& keys);
    static bool isForeWindow(HWND hwnd);
    static QString getWindowClass(HWND hwnd);
    static HWND windowFromPoint(const QPoint& pos);
    static HWND topWinFromPoint(const QPoint& pos);
    static QRect getClipCursor(void);
    static bool isCursorVisible(void);
    static bool isUnderCursor(HWND Hwnd);
    static bool isPowerOn(void);
    static bool isForeFullScreen(void);
    static void setAutoRun(const QString& AppName, bool isAuto);
    static bool isAutoRun(const QString& AppName);
    static void adjustBrightness(bool isUp, int delta = 10);
    static WORD registerHotKey(HWND hwnd, UINT modifiers, UINT key, QString str, ATOM* atom);
    static bool unregisterHotKey(ATOM atom, WORD hotKeyId, HWND hwnd);
};

#endif // WIN_H
