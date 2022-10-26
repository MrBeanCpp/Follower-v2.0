#ifndef TOOLMENU_H
#define TOOLMENU_H

#include <QDialog>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <functional>
namespace Ui {
class ToolMenu;
}

class ToolMenu : public QDialog
{
    Q_OBJECT

public:
    enum TipAlign{
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

public:
    explicit ToolMenu(QWidget *parent = nullptr);
    ~ToolMenu();

    QToolButton* addMenu(QMenu* menu, const QPoint& offset);
    void setBtnText(QToolButton* btn, const QString& text);
    QToolButton* addAction(const QString& text, const QPoint& offset, std::function<void(void)> todo);
    QLabel* addLabelTip(QToolButton* target, const QString& text = "", TipAlign align = UP, int margin = 4);
    void setLabelTip(QToolButton* target, QLabel* label, const QString& text, TipAlign align, int margin = 4);
    void setCenter(QWidget* wgt, const QPoint& center);

private:
    Ui::ToolMenu *ui;

    QList<QToolButton*> btnMenuList;
    QToolButton* lastHoverBtn = nullptr;
    QTimer* timer = nullptr;

signals:
    void closed(void);
    void showed(void);

    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override;

    // QWidget interface
protected:
    virtual void showEvent(QShowEvent *event) override;

    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event) override;
};

#endif // TOOLMENU_H
