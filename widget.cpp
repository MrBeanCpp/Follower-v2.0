#include "widget.h"
#include "keystate.h"
#include "ui_widget.h"
#include <QDebug>
#include <QDesktopWidget>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSettings>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QtWin>
#include <cmath>
#include <QMimeData>
#include <QClipboard>
#include <QToolTip>
#include "updateform.h"
#include "shortcutdia.h"
#define GetKey(X) (GetAsyncKeyState(X) & 0x8000)
Widget::Widget(QWidget* parent)
    : QWidget(parent), ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    QtWin::taskbarDeleteTab(this); //删除任务栏图标
    setWindowOpacity(0.8);
    setFocusPolicy(Qt::StrongFocus); //有焦点才能SwitchToThisWindow
    qApp->setQuitOnLastWindowClosed(false);
    setStyleSheet("QWidget#Widget{background-color:rgb(15,15,15);}");
    resize(StateSize(MOVE));

    winPos = geometry().center();
    Hwnd = HWND(winId());
    hasPower = true; //用以强调断电

    //initSizeTimer();

    updateScreenInfo();
    connect(pScreen, &QScreen::availableGeometryChanged, this, &Widget::updateScreenInfo);

    lineEdit = new CodeEditor(StateSize(INPUT).width() - 2 * Margin, StateSize(INPUT).height() - 2 * Margin, this);
    lineEdit->move(Margin, Margin);
    lineEdit->silent();

    setState(MOVE); //在lineEdit之前调用，导致野指针
    readIni(); //读取配置文件 before Init_SystemTray
    Init_SystemTray();
    //setTeleportMode(AUTO); //自动判断
    connect(&lineEdit->executor, &Executor::changeTeleportMode, this, [=](int mode) {
        setTeleportMode(TeleportMode(mode)); //static_cast
    });

    connect(lineEdit, &CodeEditor::reportResize, this, [=](int w, int h) {
        int W = w >= 0 ? w + 2 * Margin : width();
        int H = h >= 0 ? h + 2 * Margin : height();
        resize(W, H);
        //changeSizeSlow(QSize(W, H), 6, true);
        //changeSizeEx(QSize(W, H));
    });

    timer_move = new QTimer(this); //control moveing & state changing
    connect(timer_move, &QTimer::timeout, this, &Widget::updateWindow);
    timer_move->start(10);

    QTimer* timer_beat = new QTimer(this); //心跳，防止假死
    connect(timer_beat, &QTimer::timeout, this, [=]() {
        if (!isState(INPUT)) //INPUT状态update会导致拖拽卡顿
            update(); //更新时间显示

        sys->inputM->checkAndSetEn(); //En输入法//降低更新频率

        //        if (hasPower && !(hasPower = Win::isPowerOn())) //降低更新频率,防止反复调用 Win API
        //            sys->sysTray->showMessage("Little Tip", "Power Down");
        //        else if (!hasPower && (hasPower = Win::isPowerOn()))
        //            sys->sysTray->showMessage("Little Tip", "Power Up");
    });
    timer_beat->start(1000);

    connect(&lineEdit->executor, &Executor::askHide, this, [=]() {
        hide();
        timer_move->stop();
    });
    connect(&lineEdit->executor, &Executor::askShow, this, [=]() {
        show();
        QTimer::singleShot(100, this, [=]() { timer_move->start(); }); //防止Esc来不及抬起，导致INPUT->STILL
        sys->inputM->setEnMode(Hwnd);
        SwitchToThisWindow(Hwnd, true);
    });
    connect(lineEdit, &CodeEditor::returnWithoutEcho, this, [=](bool noHide) {
        setState(STILL, 2);
        if (isHideAfterExecute && !noHide) minimize(); //autoHide
    }); //命令执行后自动回复

    connect(sys->sysTray, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            winPos = Screen.center();
            //show();
            qDebug() << timer_move->isActive() << timer_move->remainingTime();
            qDebug() << timer_beat->isActive() << timer_beat->remainingTime();
            setWindowState(Qt::WindowActive);
            SwitchToThisWindow(Hwnd, true);
        }
    });

    connect(sys->gTimer, &GapTimer::timeout, this, [=]() {
        if (sys->gTimer->inGap(QDateTime(QDate::currentDate(), QTime(0, 0, 0)))) //只用Time无法比较区分两天(过零点)
            sys->sysTray->showMessage("Little Tip", "It's Time to Sleep");
    });

    timer_audioTip = new QTimer(this);
    timer_audioTip->setSingleShot(true);
    timer_audioTip->callOnTimeout([=]() {
        QToolTip::showText(QCursor::pos(), QString("Audio >> %1").arg(Win::activeAudioOutputDevice()), this, QRect(), 60000);
    });

    hPowerNotify = RegisterSuspendResumeNotification(Hwnd, DEVICE_NOTIFY_WINDOW_HANDLE); //注册睡眠/休眠消息

    connect(this, &Widget::powerSwitched, this, [=](bool isPowerOn, bool force) {
        qDebug() << "#Power:" << isPowerOn;
        if (force || hasPower != isPowerOn) {
            if (isPowerOn) {
                sys->sysTray->showMessage("Little Tip", "Power Up");
                Win::setScreenReflashRate(screenSetting.reflash[1]); //休眠恢复中可能更改不生效
                Win::setBrightness(screenSetting.brightness[1]);
            } else {
                sys->sysTray->showMessage("Little Tip", "Power Down");
                Win::setScreenReflashRate(screenSetting.reflash[0]);
                Win::setBrightness(screenSetting.brightness[0]);
            }
        }
        hasPower = isPowerOn;
    });
    emit powerSwitched(Win::isPowerOn(), true); //开机调整

    qDebug() << Win::getAvailableScreenReflashRates();
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::moveGuide(QPoint dest, QPointF& pos, qreal speed, qreal limit)
{
    const qreal Rate = DPI(speed);
    qreal x = pos.x(), y = pos.y();
    qreal Dx = dest.x() - x, Dy = dest.y() - y;
    qreal Dis = sqrt(Dx * Dx + Dy * Dy);
    qreal dx = 0, dy = 0;
    QRect rect = geometry();
    if (Dis > limit && Dis)
        dx = Dx / Dis, dy = Dy / Dis;

    if (rect.left() <= 0 && dx < 0) //计算相对位移，防止直接计算的奇偶偏差
        dx = 0, x -= rect.left();
    else if (rect.right() >= validScreen.right() - 1 && dx > 0)
        dx = 0, x -= (rect.right() - (validScreen.right() - 1));

    if (rect.top() <= 0 && dy < 0)
        dy = 0, y -= rect.top();
    else if (rect.bottom() >= validScreen.bottom() - 1 && dy > 0)
        dy = 0, y -= (rect.bottom() - (validScreen.bottom() - 1));

    pos.setX(x + dx * Rate);
    pos.setY(y + dy * Rate);
    return dx || dy;
}

bool Widget::moveWindow(qreal speed)
{
    bool isMove = moveGuide(QCursor::pos(), winPos, speed);
    if (isMove && isState(MOVE) && !isMinimized())
        move(centerToLT(winPos.toPoint()));
    return isMove;
}

void Widget::getInputFocus()
{ //因为2000/XP改变了SetForegroundWindow的执行方式，不允许随便把窗口提前，打扰用户的工作。可以用附加本线程到最前面窗口的线程，从而欺骗windows。
    HWND foreHwnd = GetForegroundWindow();
    DWORD foreThreadID = GetWindowThreadProcessId(foreHwnd, NULL);
    DWORD threadID = GetCurrentThreadId();
    AttachThreadInput(threadID, foreThreadID, TRUE);
    SetForegroundWindow(Hwnd);
    SetFocus(Hwnd);
    AttachThreadInput(threadID, foreThreadID, FALSE);
}

void Widget::changeSizeSlow(QSize size, int step, bool isAuto)
{
    int W = this->width(), H = this->height();
    int width = size.width(), height = size.height();
    if ((width == W && height == H) || step <= 0) return;
    if (isAuto) { //Automatically
        step = qMax(qAbs(W - width) / StateSize(MOVE).width(), step);
        step = qMax(qAbs(H - height) / StateSize(INPUT).height(), step);
    }
    if (!hasPower) step = qMax(step, 4); //断电加速
    step = DPI(step);
    isChangingState = true;
    while (W != width || H != height) {
        W = W > width ? qMax(width, W - step) : qMin(width, W + step);
        H = H > height ? qMax(height, H - step) : qMin(height, H + step);
        resize(W, H);
        repaint(); //wallpaper GPU占用高会影响paint时间
        //Sleep(1);//放弃时间片后 os返回的时间不确定 与CPU频率占用率有关 不可靠
        QElapsedTimer t;
        t.start();
        while (t.nsecsElapsed() < 0.8e6) //1 ms = 1e6 ns
            ; //死循环延时 可烤(靠)
        //且不能进入事件循环 否则Follower的移动可能出bug
    }
    isChangingState = false;
}

void Widget::updateWindow()
{
    static QDateTime lastTime = QDateTime::currentDateTime();
    QDateTime nowTime = QDateTime::currentDateTime();
    qint64 gap = lastTime.msecsTo(nowTime);

    bool isMove;
    bool isTeleport;
    KeyState::clearLock(); //保证检测一次
    switch (state) {
    case MOVE:
        isTeleport = false;
        if (KeyState::isRelease(VK_MBUTTON, 500) && teleportMode != OFF) {
            if (teleportMode == ON || (teleportMode == AUTO && !Win::isForeFullScreen()))
                teleport(), isTeleport = true;
        } else if (isTeleportKey()) { //键盘快捷键无视全屏
            teleportKeyDown = false; //复位
            teleport(), isTeleport = true;
        }
        if (isTeleport) {
            if (tClip.lastTextChangeToNow() < 1000) { //在Teleport命令前1000ms内复制文本 视为希望翻译(用户意愿)
                setState(INPUT, 4);
                lineEdit->setPlaceholderText('!' + tClip.text()); //将setText & return 改为 placeHolder 防止擅作主张
            } else
                setState(STILL, 4); //Teleport后加速，防止伸缩动画时无法检测按键
            break;
        }

        if (!Win::isForeWindow(Hwnd) && Win::isTopMost(Hwnd)) //若焦点转移则取消置顶
            setAlwaysTop(false);

        isMove = moveWindow(qMin(2.0 * gap / timer_move->interval(), 4.0)); //STILL->MOVE的时候伸缩动画占用时间 会导致Gap太大 所以要限制
        if (!isMove && isUnderCursor())
            setState(STILL);
        break;
    case STILL:
        if (isUnderCursor() && (KeyState::isDowning(VK_LBUTTON) || KeyState::isPress(VK_SPACE))) {
            setState(INPUT, 3);
            break;
        } else if (KeyState::isRelease(VK_RBUTTON) || KeyState::isRelease(VK_ESCAPE) || KeyState::isRelease(VK_BACK)) {
            //hide(); //setWindowState(Qt::WindowMinimized);
            minimize();
            break;
        }
        if (isCursorInWindow()) break;
        isMove = moveWindow();
        if (isMove || !isUnderCursor()) setState(MOVE);
        break;
    case INPUT:
        if (((isGetMouseButton()) && !isUnderCursor()) || isTeleportKey()) {
            teleportKeyDown = false;
            setState(MOVE, 2);
        } else if ((KeyState::isRelease(VK_RBUTTON) && isUnderCursor()) || KeyState::isRelease(VK_ESCAPE) || (lineEdit->text().isEmpty() && KeyState::isDowning(VK_CONTROL) && KeyState::isRelease(VK_BACK))) {
            setState(STILL, 2); //右键||ESC||Ctrl+BackSpace&空文本
        }
        break;
    default:
        break;
    }

    lastTime = nowTime;
}

QPoint Widget::centerToLT(const QPoint& pos)
{
    return {pos.x() - width() / 2, pos.y() - height() / 2};
}

bool Widget::isCursorInWindow()
{
    return geometry().contains(QCursor::pos());
}

void Widget::setState(Widget::State toState, int step)
{
    State _state = state; //old State
    state = toState;

    //if (!isActiveWindow()) SetActiveWindow(Hwnd); //SwitchToThisWindow(Hwnd, true); //activateWindow();
    //update();

    if (toState == STILL)
        audioOuptputDev = Win::activeAudioOutputDevice(); //update

    if (_state == INPUT) {
        lineEdit->silent();
    }
    if (toState == INPUT) {
        getInputFocus(); //ensure focus
        lineEdit->wake();
        lineEdit->setFocus();
    }

    //if (_state == INPUT || toState == INPUT) step = 2; //加速
    changeSizeSlow(StateSize(toState), step);
    //changeSizeEx(StateSize(toState));
}

bool Widget::isUnderCursor()
{
    return Win::isUnderCursor(Hwnd);
}

bool Widget::isState(Widget::State _state) //确认实际状态，防止中间态迷惑判断
{
    return state == _state && !isChangingState;
}

bool Widget::isGetMouseButton()
{
    return KeyState::isPress(VK_LBUTTON) || KeyState::isPress(VK_MBUTTON) || KeyState::isPress(VK_RBUTTON);
}

bool Widget::isTeleportKey()
{
    //return GetKey(VK_CONTROL) && GetKey(VK_SHIFT) && GetKey('E');
    return teleportKeyDown;
}

void Widget::updateScreenInfo()
{
    pScreen = QGuiApplication::screens().at(0);
    validScreen = pScreen->availableGeometry();
    Screen = pScreen->geometry();
}

void Widget::teleport()
{
    winPos = QCursor::pos();
    move(centerToLT(winPos.toPoint()));
    catchFocus();
    setAlwaysTop(); //置顶
}

void Widget::setTeleportMode(Widget::TeleportMode mode)
{
    teleportMode = mode;
    writeIni("Section_1/TeleportMode", QString::number(teleportMode));
}

void Widget::catchFocus()
{
    setWindowState(Qt::WindowMinimized);
    setWindowState(Qt::WindowActive);
    activateWindow();
    SwitchToThisWindow(Hwnd, true); //冗余双保险
}

QSize Widget::StateSize(Widget::State _state)
{
    QSize size;
    switch (_state) {
    case STILL:
        size = QSize(100, 50);
        break;
    case MOVE:
        size = QSize(50, 50);
        break;
    case INPUT:
        size = QSize(200, 40);
        break;
    default:
        size = QSize(-1, -1);
    }
    return DPI(size);
}

void Widget::writeIni(const QString& key, const QVariant& value)
{
    QSettings IniSet(iniFilePath, QSettings::IniFormat);
    IniSet.setValue(key, value);
}

void Widget::readIni() //文件不存在时，读取文件不会创建，写文件才会创建
{
    QSettings IniSet(iniFilePath, QSettings::IniFormat);
    QVariant var = IniSet.value("Section_1/TeleportMode");
    if (var == QVariant())
        teleportMode = AUTO;
    else {
        int mode = var.toInt();
        teleportMode = TeleportMode(mode); //不能用setTeleportMode()，因为会写文件
    }

    UINT modifiers = IniSet.value("Shortcut/teleport_modifiers", MOD_CONTROL | MOD_SHIFT).toUInt();
    UINT key = IniSet.value("Shortcut/teleport_key", 'E').toUInt();
    HotKeyId = Win::registerHotKey(Hwnd, modifiers, key, atomStr, &Atom); //注册全局热键

    isHideAfterExecute = IniSet.value("isHideAfterExecute", true).toBool();

    screenSetting = PowerSettingDia::readScreenSettings();
}

void Widget::Init_SystemTray()
{
    QMenu* menu = new QMenu(this);
    menu->setStyleSheet("QMenu{background-color:rgb(45,45,45);color:rgb(220,220,220);border:1px solid white;}"
                        "QMenu:selected{background-color:rgb(60,60,60);}");

    QAction* act_autoStart = new QAction("isAutoStart", menu);
    QAction* act_update = new QAction("Update", menu);
    QAction* act_autoHide = new QAction("isHideAfterExecute", menu);
    QAction* act_shortcut = new QAction("Set HotKey", menu);
    QAction* act_power = new QAction("Power Setting", menu);
    QMenu* menu_audio = new QMenu("Switch AudioDev", menu);
    QAction* act_about = new QAction("About [me]?", menu);
    QAction* act_quit = new QAction("Peace Out>>", menu);

    QActionGroup* audioGroup = new QActionGroup(menu_audio);
    connect(menu_audio, &QMenu::triggered, this, [=](QAction* action) { //切换设备
        switchAudioOutputDevice(action->text());
    });
    connect(menu_audio, &QMenu::aboutToShow, this, [=]() { //更新音频设备列表
        menu_audio->clear();
        //未清空actionGroup でも大丈夫かなあ
        QStringList audioDevs {Win::validAudioOutputDevices()};
        if (audioDevs.empty()) return;
        QString curDev = audioDevs.at(0);
        std::sort(audioDevs.begin(), audioDevs.end()); //维持列表顺序恒定
        for (const auto& dev : qAsConst(audioDevs)) {
            QAction* act = menu_audio->addAction(dev);
            act->setActionGroup(audioGroup);
            act->setCheckable(true);
            if (dev == curDev)
                act->setChecked(true);
        }
    });

    menu->addAction(act_autoStart);
    menu->addAction(act_update);
    menu->addAction(act_autoHide);
    menu->addAction(act_shortcut);
    menu->addAction(act_power);
    menu->addMenu(menu_audio);
    menu->addAction(act_about);
    menu->addAction(act_quit);

    act_autoStart->setCheckable(true);
    act_autoStart->setChecked(Win::isAutoRun(AppName));
    connect(act_autoStart, &QAction::toggled, this, [=](bool checked) {
        Win::setAutoRun(AppName, checked);
        sys->sysTray->showMessage("Tip", checked ? "已添加启动项" : "已移除启动项");
    });
    connect(act_update, &QAction::triggered, this, [=]() {
        static UpdateForm* updateForm = new UpdateForm(nullptr); //不能把this作为parent 否则最小化会同步 （这不算内存泄露吧 周期同步
        updateForm->show();
    });
    act_autoHide->setCheckable(true);
    act_autoHide->setChecked(isHideAfterExecute);
    connect(act_autoHide, &QAction::toggled, this, [=](bool checked) {
        writeIni("isHideAfterExecute", isHideAfterExecute = checked);
    });
    connect(act_shortcut, &QAction::triggered, this, [=]() {
        ShortcutDia* shortcutDia = new ShortcutDia(nullptr);
        shortcutDia->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(shortcutDia, &ShortcutDia::updateShortcut, this, [=](UINT modifiers, UINT key, QString str) {
            if (Atom) Win::unregisterHotKey(Atom, HotKeyId, Hwnd);
            HotKeyId = Win::registerHotKey(Hwnd, modifiers, key, atomStr, &Atom); //注册全局热键
            sys->sysTray->showMessage("ShortCut Tip", QString("ShortCut Teleport Updated;\n%1").arg(str));
        });
        shortcutDia->show();
    });
    connect(act_power, &QAction::triggered, this, [=]() {
        PowerSettingDia* powerDia = new PowerSettingDia(screenSetting);
        powerDia->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(powerDia, &PowerSettingDia::powerSettingApply, this, [=](ScreenSetting screenSetting) {
            this->screenSetting = screenSetting;
            sys->sysTray->showMessage("PowerTip", "Power Setting has been updated");
        });
        powerDia->show();
    });

    connect(act_about, &QAction::triggered, this, [=]() {
        QMessageBox::about(this, "About Follower v2.0", "[Made by MrBeanC]\n"
                                                        "My Vegetable has exploded");
    });
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
    sys->sysTray->setContextMenu(menu);

    sys->sysTray->setToolTip("Follower v2.0");
    sys->sysTray->show();
}

void Widget::setAlwaysTop(bool bTop)
{
    if (Hwnd) //必须要用HWND_BOTTOM HWND_NOTOPMOST 无效（依旧top）：将窗口置于所有非顶层窗口之上（即在所有顶层窗口之后）如果窗口已经是非顶层窗口则该标志不起作用。
        SetWindowPos(Hwnd, bTop ? HWND_TOPMOST : HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE); //持续置顶
    //SWP_NOACTIVATE 不激活窗口。如果未设置标志，则窗口被激活, 抢占本进程（其他进程不影响）其他窗口（即使是HWND_BOTTOM）
    //导致在本进程其他窗口上teleport后，该窗口获得焦点后会被迅速抢占(setAlwaysTop(false))
    qDebug() << "setTop:" << bTop;
}

void Widget::switchAudioOutputDevice(const QString& name, bool toPre) //封装的作用是储存变量
{
    static QString lastDev;
    QStringList devs = Win::validAudioOutputDevices();
    QString curDev = Win::activeAudioOutputDevice();
    QString toDev;
    if (toPre) {
        if (devs.size() <= 1) return; //少于2个设备
        if (lastDev.isEmpty() || lastDev == curDev || !devs.contains(lastDev)) lastDev = devs[1];
        //第一次（无last）or last == cur（可能由于插拔设备 or 程序外修改） or 上个设备不存在
        toDev = lastDev;
        lastDev = curDev;
    } else {
        if (curDev == name || !devs.contains(name)) return; //ensure exist
        lastDev = curDev;
        toDev = name;
    }
    Win::setActiveAudioOutputDevice(toDev);
    audioOuptputDev = toDev; //update
    sys->sysTray->showMessage("Audio Tip", QString("Audio Output Device Changed: %1\nPress [TAB] on STILL to back").arg(toDev));
}

void Widget::minimize()
{
    setWindowState(Qt::WindowMinimized);
    setState(MOVE);
}

void Widget::paintEvent(QPaintEvent* event)
{
    QRect rect = event->rect();
    QPainter painter(this);
    painter.setPen(QColor(200, 200, 200));
    painter.drawRect(rect.left(), rect.top(), rect.right(), rect.bottom()); //直接用rect只显示一半 WTF???

    if (state == MOVE) {
        painter.setFont(QFont("Microsoft YaHei", 13, QFont::Bold));
        painter.drawText(QRect(rect.left() + 1, rect.top() + 1, rect.right(), rect.bottom()), Qt::AlignCenter, ">_<"); //& 'ㅅ' Ծ‸Ծ ´ ᵕ `
    } else if (state == STILL) {
        painter.setFont(QFont("Consolas", 14));
        painter.drawText(QRect(rect.left(), rect.top() + DPI(8), rect.right(), rect.bottom()), Qt::AlignCenter, "Need?");

        QString dev;
        if (audioOuptputDev.contains("扬声器") || audioOuptputDev.contains("Speakers", Qt::CaseInsensitive))
            dev = ""; //🔔🔊 //咳咳 学习QQ，免提就什么都不显示，节省一个图标，更清爽
        else if (audioOuptputDev.contains("耳机") || audioOuptputDev.contains("Headphones", Qt::CaseInsensitive))
            dev = "🎧";
        else
            dev = "🎚️"; //by Darli: 如果时无法识别的类型（或者自定义名称） 则balabala
        painter.setFont(QFont("Consolas", 8));
        painter.drawText(QRect(rect.left(), rect.top() + DPI(1), rect.right() - DPI(1), rect.bottom()), Qt::AlignRight | Qt::AlignTop, dev);

        painter.setFont(QFont("Consolas", 10));
        painter.drawText(QRect(rect.left(), rect.top(), rect.right(), rect.bottom() - DPI(18)), Qt::AlignCenter, QTime::currentTime().toString());
    }

    static QPen pen;
    pen.setWidth(DPI(6));
    pen.setCapStyle(Qt::RoundCap); //圆形笔头，否则方形点
    if (state == MOVE || state == STILL) { //显示相关信息标识
        if (!sys->noteEditor->isEmpty()) {
            pen.setColor(QColor(200, 0, 0));
            painter.setPen(pen);
            painter.drawPoint(DPI(QPoint(8, 8)));
        }
        if (!hasPower) {
            pen.setColor(QColor(30, 85, 150));
            painter.setPen(pen);
            painter.drawPoint(DPI(QPoint(16, 8)));
        }
        if (Hook::isKeyLock()) {
            pen.setColor(QColor(214, 197, 64));
            painter.setPen(pen);
            painter.drawPoint(DPI(QPoint(8, StateSize(MOVE).height() - 8))); //由于r为偶数，中心需要偏移 so no -1
        }
    }
}

void Widget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event)
    lineEdit->resize(width() - 2 * Margin, StateSize(INPUT).height() - 2 * Margin);
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    if (!isState(INPUT) || event->button() != Qt::LeftButton) return;
    preMousePos = event->screenPos();
    timer_move->stop(); //防止鼠标滑出，进入MOVE状态
}

void Widget::mouseMoveEvent(QMouseEvent* event)
{
    if (!isState(INPUT) || !(event->buttons() & Qt::LeftButton)) return; //move事件不能用button
    if (preMousePos.x() < 0 || preMousePos.y() < 0) return;
    QPointF delta = event->screenPos() - preMousePos;
    winPos += delta;
    move(pos() + delta.toPoint());
    preMousePos = event->screenPos();
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    preMousePos = {-1, -1}; //标记结束，防止因INPUT提前标记导致的mouseMoveEvent
    timer_move->start();

    if (isState(INPUT))
        lineEdit->setFocus(); //返回焦点
    else if (isState(STILL)){
         auto button = event->button();
        if (button == Qt::MidButton)
            switchAudioOutputDevice(QString(), true);
        else if (button == Qt::ForwardButton)
            Win::adjustBrightness(true); //+
        else if (button == Qt::BackButton)
            Win::adjustBrightness(false); //-
    }
}

void Widget::wheelEvent(QWheelEvent* event)
{
    if (isState(STILL)) {
        bool rollUp = event->angleDelta().y() > 0;
        if (event->modifiers() & Qt::ControlModifier) {
            Win::adjustBrightness(rollUp); //阻塞
            sys->sysTray->showMessage("Brightness Changed", QString("Brightness %1").arg(rollUp ? "UP" : "DOWN"), QSystemTrayIcon::Information, 500);
        } else
            Win::simulateKeyEvent(QList<BYTE> {BYTE(rollUp ? VK_VOLUME_UP : VK_VOLUME_DOWN)});
    }
    //event->delta() > 0 ? sendKey(VK_VOLUME_UP) : sendKey(VK_VOLUME_DOWN);
}

bool Widget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    MSG* msg = (MSG*)message;
    static const UINT WM_TASKBARCREATED = RegisterWindowMessageW(L"TaskbarCreated");
    if (msg->message == WM_TASKBARCREATED) { //获取任务栏重启信息，再次隐藏任务栏图标
        qDebug() << "TaskbarCreated";
        QtWin::taskbarDeleteTab(this);
        return true;
    } else if (msg->message == WM_HOTKEY) {
        qDebug() << "#hotkey";
        if (msg->wParam == HotKeyId) //直接比较 ID
            if (isState(MOVE) || isState(INPUT)) //STILL不响应
                teleportKeyDown = true;
        return true;
    } else if (msg->message == WM_POWERBROADCAST) {
        if (msg->wParam == PBT_APMRESUMEAUTOMATIC) { //需要使用RegisterSuspendResumeNotification注册后才行
            qDebug() << "#Resume from sleep | hibernate";
            sys->sysTray->showMessage("Power Tip", "#Resume from sleep | hibernate");
            timer_move->start(); //防止休眠后 timer_move 无响应
            emit powerSwitched(Win::isPowerOn(), true); //休眠恢复中 setReflashRate可能失败 所以恢复后 再次执行
        }else  if (msg->wParam == PBT_APMPOWERSTATUSCHANGE){ //检 测通断电 or 电量变化 具体需要手动检测
            qDebug() << "#PowerStatusChanged";
            emit powerSwitched(Win::isPowerOn());
        }
    }
    return false; //此处返回false，留给其他事件处理器处理
}

void Widget::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event)
    Win::unregisterHotKey(Atom, HotKeyId, Hwnd);
    UnregisterSuspendResumeNotification(hPowerNotify);
}

void Widget::keyPressEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    int key = event->key();
    switch (state) { //采用switch是为了结构清晰
    case STILL:
        if (key == Qt::Key_Tab) //长按 200ms 显示当前设备信息
            timer_audioTip->start(200);
        break;
    default:
        break;
    }
}

void Widget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->isAutoRepeat()) return;
    int key = event->key();
    switch (state) {
    case STILL:
        if (key == Qt::Key_Tab) {
            if (timer_audioTip->isActive()) { //短按TAB切换音频输出设备(≈110ms)
                timer_audioTip->stop();
                switchAudioOutputDevice(QString(), true);
                /*巨复杂写法
                 * 手动记录records  需要List而非单变量的原因是切换Menu->Action时无法获取上一个Action（所以需要>三个变量）
                 * 其实可以用过Win::activeAudioOutputDevice获取
                QString curDev = Win::activeAudioOutputDevice();
                qDebug() << "TAB" << curDev;
                QStringList devs {Win::validAudioOutputDevices()};
                int N = audioRecords.size();
                if (devs.size() > 1) {
                    QString lastDev = N ? audioRecords.at(qMax(0, N - 1)) : curDev; //处理records为空的情况
                    if (devs.contains(lastDev)) { //ensure exist
                        QString toDev = (lastDev == curDev ? devs[1] : lastDev); //避免和当前相同（第一次的情况）
                        Win::setActiveAudioOutputDevice(toDev);
                        audioRecords << toDev;
                        while (audioRecords.size() > 2) audioRecords.pop_front(); //清理过多records
                        qDebug() << audioRecords;
                    }
                }*/
            } else
                QToolTip::hideText(); //由于showText有延迟 导致有时hideText检测不到visible导致hide失败(在200ms附近)
        }
        break;
    default:
        break;
    }
}
