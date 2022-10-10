#ifndef WIN_H
#define WIN_H
#include <QPoint>
#include <QString>
#include <windows.h>
#include <QRect>
//#include <QAudioDeviceInfo>
#include <QDebug>
struct AudioDevice {
    AudioDevice() = default;
    AudioDevice(const QString& id)
        : id(id), name("")
    {
    }
    AudioDevice(const QString& id, const QString& name)
        : id(id), name(name)
    {
    }
    bool isNull(void){
        return id.isEmpty();
    }
    QString id;
    QString name;
};
QDebug operator<<(QDebug dbg, const AudioDevice& dev);
bool operator==(const AudioDevice& lhs, const AudioDevice& rhs);

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
    static void setBrightness(int brightness);
    static WORD registerHotKey(HWND hwnd, UINT modifiers, UINT key, QString str, ATOM* atom);
    static bool unregisterHotKey(ATOM atom, WORD hotKeyId, HWND hwnd);
    static bool setDefaultAudioOutputDevice(QString devID);
    static QList<AudioDevice> enumAudioOutputDevice(void);
    static AudioDevice defaultAudioOutputDevice(void);
    static void setScreenReflashRate(int rate);
    static DWORD getCurrentScreenReflashRate(void);
    static QSet<DWORD> getAvailableScreenReflashRates(void);
};

#endif // WIN_H
