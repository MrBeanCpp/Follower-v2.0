#include "sniper.h"
#include "ui_sniper.h"
#include <QCursor>
#include <QDebug>
#include <QTime>
#include "WinUtility.h"

Sniper::Sniper(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Sniper)
{
    ui->setupUi(this);
    ui->label->setToolTip("Window EXE Locator, Click & Move");
}

Sniper::~Sniper()
{
    delete ui;
}

QString Sniper::getWindowPathUnderCursor()
{
    HWND hwnd = Win::windowFromPoint(QCursor::pos());
    // this->winId(); // 该函数调用会导致自身以及父窗口分裂为一个native窗口（可被Spy++检测），导致mouseMove行为中断，mouseRelease不触发
    if (hwnd == (HWND)this->nativeParentWidget()->winId()) return QString();

    return Win::getProcessExePath(hwnd);
}

void Sniper::mousePressEvent(QMouseEvent*)
{
    qApp->setOverrideCursor(QCursor(QPixmap(":/images/aimer.png")));
    ui->label->setPixmap(QPixmap(":/images/Window-blank.png"));

    emit windowSelectStart();
}

void Sniper::mouseReleaseEvent(QMouseEvent*)
{
    qApp->restoreOverrideCursor();
    ui->label->setPixmap(QPixmap(":/images/Window-aimer.png"));

    QString exePath = getWindowPathUnderCursor();
    emit windowSelected(exePath); // must emit, must has an end
}

void Sniper::mouseMoveEvent(QMouseEvent*)
{
    QString exePath = getWindowPathUnderCursor();
    emit windowHovered(exePath);
}
