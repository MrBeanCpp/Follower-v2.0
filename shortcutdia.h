#ifndef SHORTCUTDIA_H
#define SHORTCUTDIA_H

#include <QDialog>
#include <windows.h>
namespace Ui {
class ShortcutDia;
}

class ShortcutDia : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutDia(QWidget *parent = nullptr);
    ~ShortcutDia();

private:
    Ui::ShortcutDia *ui;

    // QObject interface
public:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void updateShortcut(UINT modifiers, UINT key, QString str);
};

#endif // SHORTCUTDIA_H
