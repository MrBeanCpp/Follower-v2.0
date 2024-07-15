#ifndef SNIPER_H
#define SNIPER_H

#include <QWidget>

namespace Ui {
class Sniper;
}

class Sniper : public QWidget
{
    Q_OBJECT

public:
    explicit Sniper(QWidget *parent = nullptr);
    ~Sniper();

private:
    QString getWindowPathUnderCursor();

private:
    Ui::Sniper *ui;

signals:
    void windowSelectStart();
    void windowSelected(QString exePath);
    void windowHovered(QString exePath);

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
};

#endif // SNIPER_H
