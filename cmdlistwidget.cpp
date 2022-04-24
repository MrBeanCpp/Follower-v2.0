#include "cmdlistwidget.h"
#include <QKeyEvent>

CMDListWidget::CMDListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setStyleSheet("QListWidget{background-color:rgb(15,15,15);color:rgb(200,200,200);border:none;outline:0px;}" //透明背景会导致边框闪现
                  "QListWidget::item:selected{background-color:lightgray; color:black; }"
                  "QListWidget::item:selected:!active{background-color:gray; }");
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(this, &CMDListWidget::itemDoubleClicked, this, &CMDListWidget::itemActivedEx);
}

void CMDListWidget::addIconItems(const IconStrList& list)
{
    clear();
    for (auto& p : list) {
        QListWidgetItem* item = new QListWidgetItem(p.second, this);
        item->setSizeHint(QSize(-1, Item_H));
        if (!p.first.isNull()) item->setIcon(p.first);
        addItem(item);
    }
    setCurrentRow(0);
    adjustSizeEx();
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
    case Qt::Key_End:
        emit itemActivedEx(currentItem());
        break;
    default:
        break;
    }
    return QListWidget::keyPressEvent(event);
}
