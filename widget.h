#ifndef WIDGET_H
#define WIDGET_H

#include "codeeditor.h"
#include "path.h"
#include <QLineEdit>
#include <QTime>
#include <QTimeLine>
#include <QWidget>
#include <systemapi.h>
#include <windows.h>
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget* ui;

private:
    QPointF winPos; //center pos
    const qreal disLimit = 10;
    const int Margin = 5; //between MainWindow and LineEditor
    HWND Hwnd;
    QRect Screen;
    QRect validScreen;
    QScreen* pScreen;

    enum State {
        STILL,
        MOVE,
        INPUT
    };

    enum TeleportMode {
        ON = 1, //总是开启(ALWAYS)
        AUTO = 0, //自动判断(全屏)
        OFF = -1 //总是关闭(NEVER)
    };

    State state;
    TeleportMode teleportMode;
    CodeEditor* lineEdit = nullptr;
    QTimer* timer_move = nullptr;

    QPointF preMousePos = { -1, -1 };
    bool isChangingState = false; //改变状态中间态
    bool hasNote; //缓存结果，避免频繁读取文件增大开销
    bool hasPower;

    bool teleportKeyDown = false;
    int HotKeyId;
    ATOM Atom;

    QTimeLine* timer_w = nullptr;
    QTimeLine* timer_h = nullptr;

    //SystemAPI sys;

    const QString iniFilePath = Path::iniFile();

private:
    inline bool moveGuide(QPoint dest, QPointF& pos, qreal limit);
    inline bool moveWindow(void);
    inline void getInputFocus(void);
    inline void catchFocus(void);
    void changeSizeSlow(QSize size, int step = 1, bool isAuto = false); //auto
    void initSizeTimer(void); //废弃-----------------
    void changeSizeEx(QSize size, int dur = 200); //废弃----------------
    void updateWindow(void);
    inline QPoint centerToLeftTop(const QPoint& pos);
    inline QPoint leftTopToCenter(const QPoint& pos);
    inline bool isCursorInWindow(void);
    inline void setState(State toState, int step = 1);
    inline bool isTopWindow(void);
    inline HWND topWindowFromPoint(QPoint pos);
    inline bool isDownToCursor(void);
    inline void sendKey(BYTE bVK);
    inline bool isInPower(void);
    inline bool isState(State _state);
    inline bool isGetMouseButton(void); //LBUTTON || MBUTTON || RBUTTON
    inline bool isTeleportKey(void); //Ctrl+Shift+E
    inline void updateScreenInfo(void);
    inline void teleport(void); //瞬移
    inline void setTeleportMode(TeleportMode mode);
    bool isOtherFullScreen(void);
    QSize StateSize(State _state);
    void wrtieIni(void);
    void readIni(void);
    void Init_SystemTray(void);
    void registerHotKey(void);

    // QWidget interface
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // QWidget interface
protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent* event) override;
};
#endif // WIDGET_H
