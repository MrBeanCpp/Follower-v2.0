#include "cmdlistwidget.h"
#include <QKeyEvent>
#include <QApplication>
#include <QtConcurrent>
#include <QTime>

CMDListWidget::CMDListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setStyleSheet("QListWidget{background-color:rgb(15,15,15);color:rgb(200,200,200);border:none;outline:0px;}" //透明背景会导致边框闪现
                  "QListWidget::item:selected{background-color:lightgray; color:black; }"
                  "QListWidget::item:selected:!active{background-color:gray; }");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setUniformItemSizes(true); //统一优化

    connect(this, &CMDListWidget::itemDoubleClicked, this, &CMDListWidget::itemActivedEx);
}

void CMDListWidget::addIconItems(const IconStrList& list) //貌似加载不同的图标会降低绘制速度（可能影响优化）
{
    static QTime tModify;
    if (listCache == list) { //防止重复绘制
        qDebug() << "Hit Cache";
        return;
    }
    tModify = QTime::currentTime(); //上次被修改的时间
    listCache = list;

    this->clear();
    for (auto& p : list) {
        static QIcon nullIcon(":/images/web.png"); //默认图标
        QListWidgetItem* item = new QListWidgetItem(p.second, this);
        item->setSizeHint(QSize(0, Item_H));
        //item->setIcon(p.first); //QIcon()==无图标
        if (!p.first.isNull())
            item->setIcon(nullIcon);
        addItem(item);
    }
    setCurrentRow(0);
    adjustSizeEx();

    QTimer::singleShot(0, this, [=]() { //进入事件队列 在首次渲染list完成后再add Icon
        //QtConcurrent::run([=]() {
        qApp->processEvents();
        QTime t = tModify; //static 无需捕获
        int rows = count();
        if (!isVisible()) return;
        if (listCache.size() != rows) return;
        for (int i = 0; i < rows; i++) {
            if (t != tModify) return; //listCache被修改
            item(i)->setIcon(listCache[i].first); //setIcon不耗时，渲染耗时
            //qApp->processEvents();
        }
        //});
    });
}

void CMDListWidget::adjustSizeEx()
{
    int rows = count();
    int maxW = 0;
    QFontMetrics fontMetrics(font());
    for (int i = 0; i < rows; i++)
        maxW = std::max(maxW, fontMetrics.horizontalAdvance(item(i)->text()));
    resize(maxW + 2 * frameWidth() + Item_H + 15, Item_H * rows + 2 * frameWidth());
}

void CMDListWidget::selectNext()
{
    int i = currentRow() + 1;
    setCurrentRow(i % count());
}

void CMDListWidget::selectPre()
{
    int i = currentRow() - 1;
    setCurrentRow((i + count()) % count()); //循环
}

QString CMDListWidget::selectedText()
{
    return currentItem()->text();
}

void CMDListWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        emit itemActivedEx(currentItem());
        break;
    default:
        break;
    }
    return QListWidget::keyPressEvent(event);
}

void CMDListWidget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event)
    listCache.clear(); //hide时Cache失效，防止show时 Cache==list 导致不绘制
}

bool operator==(const QIcon& lhs, const QIcon& rhs) //QIcon只能用cacheKey 无预定义==
{
    return lhs.cacheKey() == rhs.cacheKey();
}
