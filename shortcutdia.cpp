#include "shortcutdia.h"
#include "ui_shortcutdia.h" // 移植时没有这个文件，重新构建项目即可自动生成
#include <QDebug>
#include <QKeyEvent>
#include <path.h>
#include <QSettings>
#include <QTimer>
#include <systemapi.h>
ShortcutDia::ShortcutDia(QWidget* parent)
    : QDialog(parent), ui(new Ui::ShortcutDia)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(DPI(size()));
    ui->lineEdit->installEventFilter(this);
}

ShortcutDia::~ShortcutDia()
{
    delete ui;
    this->disconnect(SIGNAL(updateShortcut(UINT, UINT, QString)));
}

bool ShortcutDia::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != ui->lineEdit) return false;
    if (event->type() == QEvent::KeyPress) {
        static UINT mods, key;
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

        if (modifiers == Qt::NoModifier && keyEvent->key() == Qt::Key_Return) {
            qDebug() << mods << key;
            QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
            IniSet.setValue("Shortcut/teleport_modifiers", mods);
            IniSet.setValue("Shortcut/teleport_key", key);
            qDebug() << "writen";
            emit updateShortcut(mods, key, ui->lineEdit->text());
            ui->lineEdit->setText("Successful");
            QTimer::singleShot(1200, [=]() {
                this->close();
            });
            return true;
        } else if (modifiers == Qt::NoModifier && keyEvent->key() == Qt::Key_Escape) {
            close();
            return true;
        }

        bool isCtrl = modifiers & Qt::ControlModifier;
        bool isAlt = modifiers & Qt::AltModifier;
        bool isShift = modifiers & Qt::ShiftModifier;
        bool isWin = modifiers & Qt::MetaModifier;
        key = keyEvent->nativeVirtualKey();
        //qDebug() << modifiers << key;

        mods &= 0;
        QString str;
        if (isCtrl) {
            str += "Ctrl";
            mods |= MOD_CONTROL;
        }
        if (isAlt) {
            str += str.isEmpty() ? "Alt" : "+Alt";
            mods |= MOD_ALT;
        }
        if (isWin) {
            str += str.isEmpty() ? "Win" : "+Win";
            mods |= MOD_WIN;
        }
        if (isShift) {
            str += str.isEmpty() ? "Shift" : "+Shift";
            mods |= MOD_SHIFT;
        }

        if (key >= ' ' && key <= '~') {
            if (!str.isEmpty()) str += '+';
            str += char(key);
        }

        ui->lineEdit->setText(str);
        return true;
    }
    return false;
}
