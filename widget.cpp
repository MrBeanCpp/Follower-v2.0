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
#define GetKey(X) (GetAsyncKeyState(X) & 0x8000)
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint);
    QtWin::taskbarDeleteTab(this); //删除任务栏图标
    setWindowOpacity(0.8);
    setFocusPolicy(Qt::StrongFocus); //有焦点才能SwitchToThisWindow
    qApp->setQuitOnLastWindowClosed(false);
    setStyleSheet("QWidget#Widget{background-color:rgb(15,15,15);}");
    resize(StateSize(MOVE));

    winPos = leftTopToCenter(QPoint(geometry().x(),geometry().y()));
    Hwnd = HWND(winId());
    hasPower = true; //用以强调断电

    //initSizeTimer();

    updateScreenInfo();
    connect(pScreen, &QScreen::availableGeometryChanged, this, &Widget::updateScreenInfo);

    lineEdit = new CodeEditor(StateSize(INPUT).width() - 2 * Margin, StateSize(INPUT).height() - 2 * Margin, this);
    lineEdit->move(5, 5);
    lineEdit->silent();

    hasNote = !sys->noteEditor->isEmpty();

    setState(MOVE); //在lineEdit之前调用，导致野指针
    Init_SystemTray();
    //setTeleportMode(AUTO); //自动判断
    readIni(); //读取配置文件
    registerHotKey(); //注册全局热键
    connect(&lineEdit->executor, &Executor::changeTeleportMode, [=](int mode) {
        setTeleportMode(TeleportMode(mode)); //static_cast
    });

    connect(lineEdit, &CodeEditor::reportResize, [=](int w, int h) {
        int W = w >= 0 ? w + 2 * Margin : width();
        int H = h >= 0 ? h + 2 * Margin : height();
        changeSizeSlow(QSize(W, H), 6, true);
        //changeSizeEx(QSize(W, H));
    });

    timer_move = new QTimer(this); //control moveing & state changing
    connect(timer_move, &QTimer::timeout, this, &Widget::updateWindow);
    timer_move->start(10);

    QTimer* timer_beat = new QTimer(this); //心跳，防止假死
    connect(timer_beat, &QTimer::timeout, [=]() {
        update(); //更新时间显示

        sys->inputM->checkAndSetEn(); //En输入法//降低更新频率

        if (hasPower && !(hasPower = isInPower())) //降低更新频率,防止反复调用 Win API
            sys->sysTray->showMessage("Little Tip", "Power Down");
        else if (!hasPower && (hasPower = isInPower()))
            sys->sysTray->showMessage("Little Tip", "Power Up");
    });
    timer_beat->start(1000);

    connect(&lineEdit->executor, &Executor::askHide, [=]() {
        hide();
        timer_move->stop();
    });
    connect(&lineEdit->executor, &Executor::askShow, [=]() {
        show();
        QTimer::singleShot(100, [=]() { timer_move->start(); }); //防止Esc来不及抬起，导致INPUT->STILL
        sys->inputM->setEnMode(Hwnd);
        SwitchToThisWindow(Hwnd, true);
    });
    connect(lineEdit, &CodeEditor::returnWithoutEcho, [=]() { setState(STILL, 2); }); //命令执行后自动回复

    connect(sys->sysTray, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            winPos = Screen.center();
            //show();
            setWindowState(Qt::WindowActive);
            SwitchToThisWindow(Hwnd, true);
        }
    });

    connect(sys->gTimer, &GapTimer::timeout, [=]() {
        if (sys->gTimer->inGap(QDateTime(QDate::currentDate(), QTime(0, 0, 0)))) //只用Time无法比较区分两天(过零点)
            sys->sysTray->showMessage("Little Tip", "It's Time to Sleep");
    });
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::moveGuide(QPoint dest, QPointF& pos, qreal limit)
{
    const qreal Rate = 2;
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

bool Widget::moveWindow()
{
    bool isMove = moveGuide(QCursor::pos(), winPos, disLimit);
    if (isMove && isState(MOVE) && !isMinimized())
        move(centerToLeftTop(winPos.toPoint()));
    return isMove;
}

void Widget::getInputFocus()
{ //因为2000/XP改变了SetForegroundWindow的执行方式，不允许随便把窗口提前，打扰用户的工作。可以用附加本线程到最前面窗口的线程，从而欺骗windows。
    AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), TRUE);
    SetForegroundWindow(Hwnd);
    SetFocus(Hwnd);
    AttachThreadInput(GetWindowThreadProcessId(::GetForegroundWindow(), NULL), GetCurrentThreadId(), FALSE);
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
    isChangingState = true;
    while (W != width || H != height) {
        W = W > width ? qMax(width, W - step) : qMin(width, W + step);
        H = H > height ? qMax(height, H - step) : qMin(height, H + step);
        resize(W, H);
        repaint();
        Sleep(1);
    }
    isChangingState = false;
}

void Widget::initSizeTimer() //废弃-------------------------------------------------------
{
    if (timer_w && timer_h) return;
    timer_w = new QTimeLine(500, this);
    timer_h = new QTimeLine(500, this);
    timer_w->setUpdateInterval(10);
    timer_h->setUpdateInterval(10);
    connect(timer_w, &QTimeLine::frameChanged, [=](int frame) {setFixedWidth(frame);repaint(); });
    connect(timer_h, &QTimeLine::frameChanged, [=](int frame) {setFixedHeight(frame);repaint(); });
}

void Widget::changeSizeEx(QSize size, int dur) //废弃-------------------------------------------
{
    if (timer_w || timer_h) {
        qDebug() << "timer_w|h is null";
        return;
    }
    //timer_w->stop();
    //timer_h->stop();
    timer_w->setDuration(dur);
    timer_h->setDuration(dur);
    timer_w->setFrameRange(width(), size.width());
    timer_h->setFrameRange(height(), size.height());
    timer_w->start();
    timer_h->start();
}

void Widget::updateWindow()
{
    //qDebug() << QTime::currentTime();
    bool isMove;
    bool isTeleport;
    KeyState::clearLock(); //保证检测一次
    switch (state) {
    case MOVE:
        isTeleport = false;
        if (KeyState::isRelease(VK_MBUTTON, 500) && teleportMode != OFF) {
            if (teleportMode == ON || (teleportMode == AUTO && !isOtherFullScreen()))
                teleport(), isTeleport = true;
        } else if (isTeleportKey()) { //键盘快捷键无视全屏
            teleportKeyDown = false; //复位
            teleport(), isTeleport = true;
        }
        if (isTeleport) {
            setState(STILL, 4); //Teleport后加速，防止伸缩动画时无法检测按键
            break;
        }
        isMove = moveWindow();
        if (!isMove && isDownToCursor())
            setState(STILL);
        break;
    case STILL:
        if (isDownToCursor() && (KeyState::isDowning(VK_LBUTTON) || KeyState::isPress(VK_SPACE))) {
            setState(INPUT, 2);
            break;
        } else if (KeyState::isRelease(VK_RBUTTON) || KeyState::isRelease(VK_ESCAPE) || KeyState::isRelease(VK_BACK)) {
            //hide(); //setWindowState(Qt::WindowMinimized);
            setWindowState(Qt::WindowMinimized);
            setState(MOVE);
            break;
        }
        if (isCursorInWindow()) break;
        isMove = moveWindow();
        if (isMove || !isDownToCursor()) setState(MOVE);
        break;
    case INPUT:
        if (((isGetMouseButton()) && !isDownToCursor()) || isTeleportKey()) {
            teleportKeyDown = false;
            setState(MOVE, 2);
        } else if ((KeyState::isRelease(VK_RBUTTON) && isDownToCursor()) || KeyState::isRelease(VK_ESCAPE) || (lineEdit->text().isEmpty() && KeyState::isDowning(VK_CONTROL) && KeyState::isRelease(VK_BACK))) {
            setState(STILL, 2); //右键||ESC||Ctrl+BackSpace&空文本
        }
        break;
    default:
        break;
    }
}

QPoint Widget::centerToLeftTop(const QPoint& pos)
{
    return { pos.x() - width() / 2, pos.y() - height() / 2 };
}

QPoint Widget::leftTopToCenter(const QPoint& pos)
{
    return { pos.x() + width() / 2, pos.y() + height() / 2 };
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

    hasNote = !sys->noteEditor->isEmpty(); //降低更新频率 防止反复读文件

    if (_state == INPUT) {
        lineEdit->clear();
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

bool Widget::isTopWindow()
{
    return GetForegroundWindow() == Hwnd;
}

HWND Widget::topWindowFromPoint(QPoint pos)
{
    HWND hwnd = WindowFromPoint({ pos.x(), pos.y() });
    while (GetParent(hwnd) != NULL)
        hwnd = GetParent(hwnd);
    return hwnd;
}

bool Widget::isDownToCursor()
{
    HWND hwnd = topWindowFromPoint(QCursor::pos());
    return hwnd == Hwnd;
}

void Widget::sendKey(BYTE bVK)
{
    keybd_event(bVK, 0, 0, 0);
    keybd_event(bVK, 0, KEYEVENTF_KEYUP, 0);
}

bool Widget::isInPower()
{
    static SYSTEM_POWER_STATUS powerSatus;
    GetSystemPowerStatus(&powerSatus);
    return powerSatus.ACLineStatus;
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
    move(centerToLeftTop(winPos.toPoint()));
    catchFocus();
}

void Widget::setTeleportMode(Widget::TeleportMode mode)
{
    teleportMode = mode;
    wrtieIni();
}

void Widget::catchFocus()
{
    setWindowState(Qt::WindowMinimized);
    setWindowState(Qt::WindowActive);
    activateWindow();
    SwitchToThisWindow(Hwnd, true); //冗余双保险
}

bool Widget::isOtherFullScreen()
{
    HWND Hwnd = GetForegroundWindow();

    HWND H_leftBottom = topWindowFromPoint(Screen.bottomLeft()); //获取左下角像素所属窗口，非全屏是任务栏
    if (Hwnd != H_leftBottom) return false;

    RECT Rect;
    GetWindowRect(Hwnd, &Rect);
    if (Rect.right - Rect.left >= Screen.width() && Rect.bottom - Rect.top >= Screen.height()) //确保窗口大小(二重验证)
        return true;
    return false;
}

QSize Widget::StateSize(Widget::State _state)
{
    switch (_state) {
    case STILL:
        return { 100, 50 };
    case MOVE:
        return { 50, 50 };
    case INPUT:
        return { 200, 40 };
    default:
        return { -1, -1 };
    }
}

void Widget::wrtieIni()
{
    QSettings IniSet(iniFilePath, QSettings::IniFormat);
    IniSet.setValue("Section_1/TeleportMode", QString::number(teleportMode));
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
}

void Widget::Init_SystemTray()
{
    QMenu* menu = new QMenu(this);
    menu->setStyleSheet("QMenu{background-color:rgb(15,15,15);color:rgb(220,220,220);}"
                        "QMenu:selected{background-color:rgb(60,60,60);}");
    QAction* act_about = new QAction("About", menu);
    QAction* act_quit = new QAction("Peace Out", menu);
    menu->addAction(act_about);
    menu->addAction(act_quit);
    connect(act_about, &QAction::triggered, [=]() {
        QMessageBox::about(this, "About Follower v2.0", "[Made by MrBeanC]\n"
                                                        "My Vegetable has exploded");
    });
    connect(act_quit, &QAction::triggered, qApp, &QApplication::quit);
    sys->sysTray->setContextMenu(menu);

    sys->sysTray->setToolTip("Follower v2.0");
    sys->sysTray->show();
}

void Widget::registerHotKey()
{
    Atom = GlobalAddAtomA("Follower_MrBeanC_Teleport");
    HotKeyId = Atom - 0xC000;
    //Ctrl+Shift+E 瞬移全局快捷键
    bool bHotKey = RegisterHotKey((HWND)winId(), HotKeyId, MOD_CONTROL | MOD_SHIFT, 'E'); //只会响应一个线程的热键，因为响应后，该消息被从队列中移除，无法发送给其他窗口
    qDebug() << "RegisterHotKey: " << Atom << HotKeyId << bHotKey;
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
        painter.drawText(QRect(rect.left(), rect.top() + 8, rect.right(), rect.bottom()), Qt::AlignCenter, "Need?");

        painter.setFont(QFont("Consolas", 10));
        painter.drawText(QRect(rect.left(), rect.top(), rect.right(), rect.bottom() - 18), Qt::AlignCenter, QTime::currentTime().toString());
    }

    static QPen pen;
    pen.setWidth(6);
    pen.setCapStyle(Qt::RoundCap); //圆形笔头，否则方形点
    if (state == MOVE || state == STILL) { //显示相关信息标识
        if (hasNote) {
            pen.setColor(QColor(200, 0, 0));
            painter.setPen(pen);
            painter.drawPoint(8, 8);
        }
        if (!hasPower) {
            pen.setColor(QColor(30, 85, 150));
            painter.setPen(pen);
            painter.drawPoint(16, 8);
        }
        if (Hook::isKeyLock()) {
            pen.setColor(QColor(214, 197, 64));
            painter.setPen(pen);
            painter.drawPoint(8, StateSize(MOVE).height() - 8); //由于r为偶数，中心需要偏移 so no -1
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
    preMousePos = { -1, -1 }; //标记结束，防止因INPUT提前标记导致的mouseMoveEvent
    timer_move->start();
    lineEdit->setFocus(); //返回焦点
}

void Widget::wheelEvent(QWheelEvent* event)
{
    if (isState(STILL))
        event->delta() > 0 ? sendKey(VK_VOLUME_UP) : sendKey(VK_VOLUME_DOWN);
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
        UINT modify = (UINT)LOWORD(msg->lParam);
        UINT key = (UINT)HIWORD(msg->lParam);
        if (modify == (MOD_CONTROL | MOD_SHIFT) && key == 'E') //瞬移全局快捷键
            if (isState(MOVE) || isState(INPUT)) //STILL不响应
                teleportKeyDown = true;
        return true;
    }
    return false; //此处返回false，留给其他事件处理器处理
}

void Widget::closeEvent(QCloseEvent* event)
{
    Q_UNUSED(event)
    qDebug() << "GlobalDeleteAtom:" << GlobalDeleteAtom(Atom); //=0
    qDebug() << "UnregisterHotKey:" << UnregisterHotKey((HWND)winId(), HotKeyId); //!=0
}
