#ifndef TABLEBASE_H
#define TABLEBASE_H

#include "ui_tablebase.h"
#include <QAction>
#include <QDialog>
#include <QMenu>
#include <QString>
#include "systemapi.h"
namespace Ui {
class TableBase;
}

class TableBase : public QDialog
{
    Q_OBJECT

public:
    using TableList = QList<QStringList>;

    explicit TableBase(const QString& Path, QWidget* parent = nullptr);
    ~TableBase();

protected:
    QString path;

    Ui::TableBase* ui;
    QTableWidget* table; //ui->tableWidget别名，简化书写

    TableList oldList; //getContentList()更新后读取即可启动close检查
    TableList newList; //closeEvent自动读取

protected:
    QMenu* deleteMenu; //右键点击
    QAction* deleteAction; //删除事件（如需要其它事件，可新定义）

    QTableWidgetItem* dropItem = nullptr;
    QString dropPath;

private slots:
    void showMenu(QPoint pos); //显示菜单
    void actDelete(QAction* act); //得知菜单当前的位置并删除

public:
    virtual void readFile(const QString& path) = 0;
    virtual void writeFile(const QString& path) = 0;
    virtual void newLine(void);
    virtual TableList getContentList(void); //加入read/writeFile效率更高，但分离使用更便捷
    virtual bool exChangeRow(int row, int NewRow);
    virtual void whatsThisEvent(void);

    static bool isBlank(const QString& str);
    bool isItemValid(const QTableWidgetItem* item);
    void deleteItem(QTableWidgetItem* item);
    void deleteSelectedItems(void);
    void moveSelectedRow(bool isUp = true);
    void clear(void);

    // QWidget interface
protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    // QObject interface
public:
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;
};

#endif // TABLEBASE_H
