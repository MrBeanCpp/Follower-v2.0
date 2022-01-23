#include "tablebase.h"
#include "ui_tablebase.h"
#include <QDebug>
#include <QFile>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QWhatsThis>
#include <windows.h>
TableBase::TableBase(const QString& Path, QWidget* parent)
    : QDialog(parent), path(Path), ui(new Ui::TableBase)
{
    ui->setupUi(this);

    //setFixedSize(size());
    setWhatsThis("What's this?");
    table = ui->tableWidget; //ui->tableWidget别名，简化书写

    setAcceptDrops(true); //QTableWidget不接受拖放，只能由父窗体代劳

    ui->btn_save->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));

    QFile qss(":/tableStyle.qss");
    if (qss.open(QFile::ReadOnly)) {
        setStyleSheet(qss.readAll());
        qss.close();
    }

    table->setContextMenuPolicy(Qt::CustomContextMenu);
    deleteMenu = new QMenu(table); //右键点击菜单
    deleteAction = new QAction("Delete", this); //删除事件
    deleteMenu->addAction(deleteAction); //将action添加到菜单内

    table->setRowCount(0);
    table->verticalScrollBar()->setSingleStep(6);

    table->installEventFilter(this);
    this->installEventFilter(this);

    connect(table, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(deleteMenu, SIGNAL(triggered(QAction*)), this, SLOT(actDelete(QAction*)));

    connect(ui->btn_add, &QToolButton::clicked, this, &TableBase::newLine);
    connect(ui->btn_save, &QToolButton::clicked, [=]() {
        ui->btn_save->setFocus(); //改变焦点，使编辑内容保存
        writeFile(path);
    });
}

TableBase::~TableBase()
{
    delete ui;
}
void TableBase::showMenu(QPoint pos)
{
    Q_UNUSED(pos)
    deleteMenu->exec(QCursor::pos());
}

void TableBase::actDelete(QAction* act)
{
    if (act->text() == "Delete") //看选中了删除这个菜单
        deleteSelectedItems();
}

bool TableBase::isBlank(const QString& str)
{
    return str.simplified() == "";
}

bool TableBase::isItemValid(const QTableWidgetItem* item)
{
    return item != nullptr && !isBlank(item->text());
}

void TableBase::deleteItem(QTableWidgetItem* item)
{
    if (item == nullptr) return;
    table->removeRow(table->row(item));
}

void TableBase::deleteSelectedItems()
{
    QList<QTableWidgetItem*> selectItems = table->selectedItems(); //获取所有选定项目的列表//不包含empty cell
    for (auto item : selectItems)
        deleteItem(item); //删除掉了表格信息
}

void TableBase::moveSelectedRow(bool isUp)
{
    int row = table->currentRow();
    int NewRow = row;
    if (isUp) {
        if (row - 1 >= 0) NewRow--;
    } else {
        if (row + 1 < table->rowCount()) NewRow++;
    }
    exChangeRow(row, NewRow);
    table->selectRow(NewRow); //使得连续移动成为可能
}

void TableBase::newLine()
{
    int row = table->rowCount(), col = table->columnCount();
    table->setRowCount(row + 1);
    for (int i = 0; i < col; i++)
        table->setItem(row, i, new QTableWidgetItem); //否则selectedItems()不识别
    table->clearSelection(); //取消现有select，防止因Ctrl导致多行选中
    table->selectRow(row); //设置选中，直接跳转到新行
}

TableBase::TableList TableBase::getContentList()
{
    TableList list;
    int row = table->rowCount(), col = table->columnCount();
    for (int i = 0; i < row; i++) {
        QStringList colList;
        for (int j = 0; j < col; j++) {
            QTableWidgetItem* item = table->item(i, j);
            if (item != nullptr && !isBlank(item->text()))
                colList << item->text();
            else
                colList << "";
        }
        if (colList.count("") < col) list << colList; //剔除空行
    }
    return list;
}

bool TableBase::exChangeRow(int row, int NewRow)
{
    if (row == NewRow) return false;
    for (int i = 0; i < table->columnCount(); i++) {
        QTableWidgetItem* item_row = table->takeItem(row, i);
        QTableWidgetItem* item_NewRow = table->takeItem(NewRow, i);

        table->setItem(row, i, item_NewRow);
        table->setItem(NewRow, i, item_row);
    }
    return true;
}

void TableBase::whatsThisEvent()
{
    QMessageBox::about(this, "what's this?", "So What's This?");
}

void TableBase::clear()
{
    table->clearContents();
    table->setRowCount(0);
}

void TableBase::resizeEvent(QResizeEvent* event) //用布局崩溃，原因不明
{
    Q_UNUSED(event)
    static const int Margin = 50;
    int c_X = this->width() - Margin / 2, c_Y = this->height() / 2; //；蓝色显眼
    table->resize(this->width() - Margin, this->height());
    ui->btn_add->move(c_X - ui->btn_add->width() / 2, c_Y - ui->btn_add->height() / 2);
    ui->btn_save->move(c_X - ui->btn_save->width() / 2, height() - 25);
}

void TableBase::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction(); //接受，使图标变化
        dropPath = event->mimeData()->urls().at(0).toLocalFile().replace('/', '\\');
        SwitchToThisWindow(HWND(winId()), true); //取得焦点
    }
}

void TableBase::dragMoveEvent(QDragMoveEvent* event)
{
    QPoint Pos = event->pos() - table->viewport()->pos(); //手动矫正坐标
    dropItem = table->itemAt(Pos);
    table->setCurrentItem(dropItem);
}

void TableBase::dropEvent(QDropEvent* event)
{
    Q_UNUSED(event)
    dropItem->setText(dropPath);
}

void TableBase::closeEvent(QCloseEvent* event)
{
    if (oldList.isEmpty()) return; //若子类没有去获取oldList，则无法比较
    newList = getContentList();
    if (oldList == newList) return;
    QMessageBox::StandardButton button = QMessageBox::question(this, "真面目な問題", "Save or Not?", QMessageBox::Save | QMessageBox::No | QMessageBox::Cancel);
    if (button == QMessageBox::Save)
        writeFile(path);
    else if (button == QMessageBox::Cancel)
        event->ignore();
}

bool TableBase::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched)

    if (event->type() != QEvent::KeyPress) return false;
    QKeyEvent* keyevent = static_cast<QKeyEvent*>(event);
    if (keyevent->matches(QKeySequence::Save)) { //Ctrl+S
        ui->btn_save->setFocus(); //改变焦点，使编辑内容保存
        writeFile(path);
    } else if (keyevent->key() == Qt::Key_Escape) {
        close();
    } else if (keyevent->key() == Qt::Key_Backspace) {
        deleteSelectedItems();
    } else if (keyevent->key() == Qt::Key_Up) {
        moveSelectedRow(true); //Up
    } else if (keyevent->key() == Qt::Key_Down) {
        moveSelectedRow(false); //Down
    } else if (keyevent->matches(QKeySequence::New)) { //Ctrl+N
        newLine();
    } else
        return false;
    return true;
}

bool TableBase::event(QEvent* event)
{
    if (event->type() == QEvent::EnterWhatsThisMode && !isHidden()) {
        QWhatsThis::leaveWhatsThisMode(); //使光标正常化
        whatsThisEvent();
        return true;
    }
    return QDialog::event(event);
}
