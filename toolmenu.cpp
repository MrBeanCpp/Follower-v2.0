#include "toolmenu.h"
#include "ui_toolmenu.h"
#include <QDebug>
#include <QMouseEvent>
#include <QTime>
#include <QTimer>
#include <windows.h>
#include <QScreen>
ToolMenu::ToolMenu(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ToolMenu)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground); //启用半透明背景//也可以实现背景穿透！！！

    resize(qApp->primaryScreen()->availableSize());

    timer = new QTimer(this);
    timer->callOnTimeout([=](){
        if(qApp->focusWidget() == nullptr) //焦点离开本程序后关闭
            this->close();
        QWidget* wgt = qApp->widgetAt(QCursor::pos());
        if(wgt && wgt->inherits("QToolButton")){ //wgt->metaObject()->className() == QStringLiteral("QToolButton")
            QToolButton* btn = static_cast<QToolButton*>(wgt);
            btn->setStyleSheet("background-color:rgb(29,84,92);");
            if(lastHoverBtn && lastHoverBtn != btn)
                lastHoverBtn->setStyleSheet("");
            lastHoverBtn = btn;
        }else if(lastHoverBtn)
            lastHoverBtn->setStyleSheet("");
    });
    timer->setInterval(50);
}

ToolMenu::~ToolMenu()
{
    delete ui;
}

QToolButton* ToolMenu::addMenu(QMenu *menu, const QPoint &offset)
{
    Q_ASSERT(menu);
    QToolButton* btn = new QToolButton(this);
    //if(!menu->actions().isEmpty()){ // empty也得接受 考虑AudioMenu一开始就是NULL aboutToShow才开始加载
        btnMenuList << btn;
        btn->setMenu(menu);
    //}
    btn->setText(menu->title());
    btn->adjustSize(); //很重要 否则size不正确
    setCenter(btn, rect().center() + offset);
    return btn;
}

QToolButton* ToolMenu::addAction(const QString &text, const QPoint &offset, std::function<void ()> todo)
{
    QToolButton* btn = new QToolButton(this);
    connect(btn, &QToolButton::clicked, todo);
    btn->setText(text);
    btn->adjustSize(); //很重要 否则size不正确
    setCenter(btn, rect().center() + offset);
    return btn;
}

QLabel *ToolMenu::addLabelTip(QToolButton *target, const QString &text, TipAlign align, int margin)
{
    QLabel* label = new QLabel("", this);
    label->setAlignment(Qt::AlignCenter);
    setLabelTip(target, label, text, align, margin);
    return label;
}

void ToolMenu::setLabelTip(QToolButton *target, QLabel *label, const QString &text, TipAlign align, int margin)
{
    label->setText(text);
    label->adjustSize();
    QPoint offset;
    switch (align) {
    case UP:
        offset = -QPoint(0, target->height()/2 + label->height()/2 + margin);
        break;
    case DOWN:
        offset = QPoint(0, target->height()/2 + label->height()/2 + margin);
        break;
    case LEFT:
        offset = -QPoint(target->width()/2 + label->width()/2 + margin, 0);
        break;
    case RIGHT:
        offset = QPoint(target->width()/2 + label->width()/2 + margin, 0);
        break;
    }
    setCenter(label, target->geometry().center() + offset);
}

void ToolMenu::setCenter(QWidget *wgt, const QPoint &center)
{
    auto rect = wgt->geometry();
    rect.moveCenter(center);
    wgt->move(rect.topLeft());
}

bool ToolMenu::eventFilter(QObject *watched, QEvent *event)
{
//    qDebug() << watched << event;
    if((event->type() == QEvent::MouseButtonRelease) ||
        (event->type() == QEvent::KeyRelease && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Shift)){

        QWidget* wgt = qApp->widgetAt(QCursor::pos());
        if(wgt && wgt->inherits("QMenu")){
            QMenu* menu = static_cast<QMenu*>(wgt);
            QAction* act = menu->actionAt(menu->mapFromGlobal(QCursor::pos()));
            if(act){
//                 qDebug() << "Act" << act->text();
                 act->trigger();
            }
        }else if(wgt && wgt->inherits("QToolButton")){
            QToolButton *btn = static_cast<QToolButton*>(wgt);
            if(btn->menu() == nullptr)
                btn->click();
        }
        this->close();
        return true;
    }

    if(event->type() == QEvent::MouseMove){
        QWidget* wgt = qApp->widgetAt(QCursor::pos());
        if(wgt && wgt->inherits("QToolButton")){
            QToolButton* btn = static_cast<QToolButton*>(wgt);

            for(auto _btn: qAsConst(btnMenuList))
                if(_btn != btn)
                    _btn->menu()->close();

            if(btn->isVisible()){
                //btn->setFocus();
//                QTime t = QTime::currentTime();
                btn->showMenu();
                //阻塞（类似exec 子事件循环） 所以会导致QTimer停止 不能放在Timer里 在menu close之后时间才会开始流动
                //而在eventFilter中实际上是一个func被阻塞 qApp开启另一个func
//                qDebug() << "showMenu" << t;
            }
        }
    }
    return QDialog::eventFilter(watched, event); //调用父类eventFilter 否则可能出乎意料（崩溃）
}

void ToolMenu::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    qApp->installEventFilter(this);
    timer->start();
    setCenter(this, QCursor::pos());

    emit showed();
}

void ToolMenu::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    qApp->removeEventFilter(this);
    timer->stop();

    for(auto btn: qAsConst(btnMenuList))
        btn->menu()->close();

    if(lastHoverBtn)
        lastHoverBtn->setStyleSheet("");

    emit closed();
}

