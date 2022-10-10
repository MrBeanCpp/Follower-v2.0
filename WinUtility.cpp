#include "WinUtility.h"
#include <QMessageBox>
#include <cstring>
#include <psapi.h>
#include <QScreen>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QProcess>
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"

QDebug operator<<(QDebug dbg, const AudioDevice& dev)
{
    dbg << dev.id << " " << dev.name;
    return dbg;
}
bool operator==(const AudioDevice& lhs, const AudioDevice& rhs)
{
    return lhs.id == rhs.id;
}

void Win::setAlwaysTop(HWND hwnd, bool isTop)
{
    SetWindowPos(hwnd, isTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW); //持续置顶
}

void Win::jumpToTop(HWND hwnd)
{
    setAlwaysTop(hwnd, true);
    setAlwaysTop(hwnd, false);
}

bool Win::isInSameThread(HWND hwnd_1, HWND hwnd_2)
{
    if (hwnd_1 && hwnd_2)
        return GetWindowThreadProcessId(hwnd_1, NULL) == GetWindowThreadProcessId(hwnd_2, NULL);
    return false;
}

bool Win::isTopMost(HWND hwnd)
{
    if (hwnd == nullptr) return false;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return style & WS_EX_TOPMOST;
}

QString Win::getWindowText(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    static WCHAR text[128];
    GetWindowTextW(hwnd, text, _countof(text)); //sizeof(text)字节数256 内存溢出
    return QString::fromWCharArray(text);
}

void Win::miniAndShow(HWND hwnd)
{
    ShowWindow(hwnd, SW_MINIMIZE); //组合操作不要异步 要等前一步完成
    ShowWindow(hwnd, SW_NORMAL);
}

DWORD Win::getProcessID(HWND hwnd)
{
    DWORD PID = -1; //not NULL
    GetWindowThreadProcessId(hwnd, &PID);
    return PID;
}

QString Win::getProcessName(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    DWORD PID = getProcessID(hwnd);

    static WCHAR path[128];
    HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    GetProcessImageFileNameW(Process, path, _countof(path)); //sizeof(path)字节数256 内存溢出
    CloseHandle(Process);

    QString pathS = QString::fromWCharArray(path);
    return pathS.mid(pathS.lastIndexOf('\\') + 1);
}

void Win::getInputFocus(HWND hwnd)
{
    HWND foreHwnd = GetForegroundWindow();
    DWORD foreTID = GetWindowThreadProcessId(foreHwnd, NULL);
    DWORD threadId = GetWindowThreadProcessId(hwnd, NULL); //GetCurrentThreadId(); //增加泛用性扩大到其他窗口
    if (foreHwnd == hwnd) {
        qDebug() << "#Already getFocus";
        return;
    }
    bool res = AttachThreadInput(threadId, foreTID, TRUE); //会导致短暂的Windows误认为this==QQ激活状态 导致点击任务栏图标 持续最小化（参见下方解决法案）
    qDebug() << "#Attach:" << res;
    if (res == false) { //如果遇到系统窗口而失败 只能最小化再激活获取焦点
        miniAndShow(hwnd);
    } else {
        SetForegroundWindow(foreHwnd); //刷新QQ任务栏图标状态 防止保持焦点状态 不更新 导致点击后 最小化 而非获取焦点
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(threadId, foreTID, FALSE);
    }
}

void Win::simulateKeyEvent(const QList<BYTE>& keys) //注意顺序：如 Ctrl+Shift+A 要同按下顺序相同
{
    for (auto key : keys) //按下
        keybd_event(key, 0, 0, 0);

    std::for_each(keys.rbegin(), keys.rend(), [=](BYTE key) { //释逆序放
        keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
    });
}

bool Win::isForeWindow(HWND hwnd)
{
    return GetForegroundWindow() == hwnd;
}

QString Win::getWindowClass(HWND hwnd)
{
    static WCHAR buffer[128];
    GetClassNameW(hwnd, buffer, _countof(buffer)); //sizeof字节数 会溢出
    return QString::fromWCharArray(buffer);
}

HWND Win::windowFromPoint(const QPoint& pos)
{
    return WindowFromPoint({pos.x(), pos.y()});
}

HWND Win::topWinFromPoint(const QPoint& pos)
{
    HWND hwnd = WindowFromPoint({pos.x(), pos.y()});
    while (GetParent(hwnd) != NULL)
        hwnd = GetParent(hwnd);
    return hwnd;
}

QRect Win::getClipCursor()
{
    RECT rect;
    GetClipCursor(&rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

bool Win::isCursorVisible()
{
    CURSORINFO info;
    info.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&info);
    return info.flags == CURSOR_SHOWING;
}

bool Win::isUnderCursor(HWND Hwnd)
{
    HWND hwnd = topWinFromPoint(QCursor::pos());
    return hwnd == Hwnd;
}

bool Win::isPowerOn()
{
    static SYSTEM_POWER_STATUS powerSatus;
    GetSystemPowerStatus(&powerSatus);
    return powerSatus.ACLineStatus;
}

bool Win::isForeFullScreen()
{
    QRect Screen = qApp->primaryScreen()->geometry();
    HWND Hwnd = GetForegroundWindow();

    HWND H_leftBottom = Win::topWinFromPoint(Screen.bottomLeft()); //获取左下角像素所属窗口，非全屏是任务栏
    if (Hwnd != H_leftBottom) return false;

    RECT Rect;
    GetWindowRect(Hwnd, &Rect);
    if (Rect.right - Rect.left >= Screen.width() && Rect.bottom - Rect.top >= Screen.height()) //确保窗口大小(二重验证)
        return true;
    return false;
}

void Win::setAutoRun(const QString& AppName, bool isAuto)
{
    static const QString Reg_AutoRun = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"; //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    static const QString AppPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    if (isAuto)
        reg.setValue(AppName, AppPath);
    else
        reg.remove(AppName);
}

bool Win::isAutoRun(const QString& AppName)
{
    static const QString Reg_AutoRun = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"; //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    static const QString AppPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    return reg.value(AppName).toString() == AppPath;
}

void Win::adjustBrightness(bool isUp, int delta)
{
    QProcess process;
    process.setProgram("Powershell");
    process.setArguments(QStringList() << "-Command"
                                       << QString("&{$info = Get-Ciminstance -Namespace root/WMI -ClassName WmiMonitorBrightness;"
                                                  "$monitor = Get-WmiObject -ns root/wmi -class wmiMonitorBrightNessMethods;"
                                                  "$monitor.WmiSetBrightness(0,$info.CurrentBrightness %1 %2)}")
                                              .arg(isUp ? '+' : '-')
                                              .arg(delta));
    process.start();
    process.waitForFinished();
    qDebug() << "#Changed Brightness";
}

void Win::setBrightness(int brightness)
{
    if (brightness < 0) return;

    QProcess process;
    process.setProgram("Powershell");
    process.setArguments(QStringList() << "-Command"
                                       << QString("&{$info = Get-Ciminstance -Namespace root/WMI -ClassName WmiMonitorBrightness;"
                                                  "$monitor = Get-WmiObject -ns root/wmi -class wmiMonitorBrightNessMethods;"
                                                  "$monitor.WmiSetBrightness(0, %1)}")
                                              .arg(brightness));
    process.start();
    process.waitForFinished();
    qDebug() << "#Set Brightness:" << brightness;
}

WORD Win::registerHotKey(HWND hwnd, UINT modifiers, UINT key, QString str, ATOM* atom)
{
    *atom = GlobalAddAtomA(str.toStdString().c_str());
    ATOM HotKeyId = (*atom - 0xC000) + 1; //防止return 0
    bool bHotKey = RegisterHotKey(hwnd, HotKeyId, modifiers, key); //只会响应一个线程的热键，因为响应后，该消息被从队列中移除，无法发送给其他窗口
    qDebug() << "RegisterHotKey: " << *atom << HotKeyId << bHotKey;
    return bHotKey ? HotKeyId : 0;
}

bool Win::unregisterHotKey(ATOM atom, WORD hotKeyId, HWND hwnd)
{
    bool del = GlobalDeleteAtom(atom); //=0
    bool unReg = UnregisterHotKey(hwnd, hotKeyId); //!=0
    bool ret = !del && unReg;
    qDebug() << "UnregisterHotKey:" << ret;
    return ret;
}

bool Win::setDefaultAudioOutputDevice(QString devID)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return false;
    IPolicyConfigVista* pPolicyConfig;
    ERole reserved = eConsole;

    hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID*)&pPolicyConfig);
    if (SUCCEEDED(hr)) {
        hr = pPolicyConfig->SetDefaultEndpoint(devID.toStdWString().c_str(), reserved);
        pPolicyConfig->Release();
    }
    ::CoUninitialize();
    return SUCCEEDED(hr);
}

QList<AudioDevice> Win::enumAudioOutputDevice()
{
    QList<AudioDevice> res;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return res;
    IMMDeviceEnumerator* pEnum = NULL;
    // Create a multimedia device enumerator.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (FAILED(hr)) return res;
    IMMDeviceCollection* pDevices;
    // Enumerate the output devices.
    hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices); //eRender == out; eCapture == in
    if (FAILED(hr)) return res;
    UINT count;
    hr = pDevices->GetCount(&count);
    if (FAILED(hr)) return res;
    for (UINT i = 0; i < count; i++) {
        IMMDevice* pDevice;
        hr = pDevices->Item(i, &pDevice);
        if (SUCCEEDED(hr)) {
            LPWSTR wstrID = NULL;
            hr = pDevice->GetId(&wstrID);
            if (SUCCEEDED(hr)) {
                IPropertyStore* pStore;
                hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
                if (SUCCEEDED(hr)) {
                    PROPVARIANT friendlyName;
                    PropVariantInit(&friendlyName);
                    hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
                    if (SUCCEEDED(hr)) {
                        res << AudioDevice(QString::fromWCharArray(wstrID), QString::fromWCharArray(friendlyName.pwszVal));
                        PropVariantClear(&friendlyName);
                    }
                    pStore->Release();
                }
            }
            pDevice->Release();
        }
    }
    pDevices->Release();
    pEnum->Release();
    ::CoUninitialize();
    return res;
}

AudioDevice Win::defaultAudioOutputDevice()
{
    AudioDevice res;
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) return res;
    IMMDeviceEnumerator* pEnum = NULL;
    // Create a multimedia device enumerator.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
    if (FAILED(hr)) return res;
    // Enumerate the output devices.
    IMMDevice* pDevice;
    hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice); //eRender == out; eCapture == in
    if (FAILED(hr)) return res;
    LPWSTR wstrID = NULL;
    hr = pDevice->GetId(&wstrID);
    if (SUCCEEDED(hr)) {
        IPropertyStore* pStore;
        hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
        if (SUCCEEDED(hr)) {
            PROPVARIANT friendlyName;
            PropVariantInit(&friendlyName);
            hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
            if (SUCCEEDED(hr)) {
                res.id = QString::fromWCharArray(wstrID);
                res.name = QString::fromWCharArray(friendlyName.pwszVal);
                PropVariantClear(&friendlyName);
            }
            pStore->Release();
        }
    }
    pDevice->Release();
    pEnum->Release();
    ::CoUninitialize();
    return res;
}

void Win::setScreenReflashRate(int rate)
{
    if (rate < 0) return;
    if (rate == (int)getCurrentScreenReflashRate()) return;

    /*QRes
    QProcess pro;
    pro.setProgram("QRes");
    pro.setArguments(QStringList() << QString("/r:%1").arg(rate));
    pro.start();
    pro.waitForFinished(8000);
    */
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDisplayFrequency = rate;
    lpDevMode.dmFields = DM_DISPLAYFREQUENCY;
    LONG ret = ChangeDisplaySettings(&lpDevMode, CDS_UPDATEREGISTRY); //&保存于注册表 如果使用0，会导致Apex时有可能重置回60HZ（注册表
    qDebug() << "#Change Screen Reflash Rate(API):" << rate << (ret == DISP_CHANGE_SUCCESSFUL) << ret;
}

DWORD Win::getCurrentScreenReflashRate()
{
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDriverExtra = 0;
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode);
    return lpDevMode.dmDisplayFrequency;
}

QSet<DWORD> Win::getAvailableScreenReflashRates()
{
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDriverExtra = 0;
    int iModeNum = 0;
    QSet<DWORD> list;
    while (EnumDisplaySettings(NULL, iModeNum++, &lpDevMode)) //about 98 modes(not only 2 reflash rates)
        list << lpDevMode.dmDisplayFrequency;
    return list;
}
