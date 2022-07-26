#include "shortcutdia.h"
#include "ui_shortcutdia.h" // 移植时没有这个文件，重新构建项目即可自动生成
#include <QDebug>
#include <QKeyEvent>
#include <path.h>
#include <QSettings>
#include <QTimer>
#include <systemapi.h>
#include <QMetaEnum>
ShortcutDia::ShortcutDia(QWidget* parent)
    : QDialog(parent), ui(new Ui::ShortcutDia)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(DPI(size()));
    ui->lineEdit->setAttribute(Qt::WA_InputMethodEnabled, false); //屏蔽输入法 否则输入法是检测不到KeyPress事件的
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
        static UINT mods, vkey;
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        Qt::Key key = Qt::Key(keyEvent->key());
        QString keyStr = QMetaEnum::fromType<Qt::Key>().valueToKey(key);

        if ((modifiers == Qt::NoModifier && key == Qt::Key_Return) || (modifiers == Qt::KeypadModifier && key == Qt::Key_Enter)) { //这俩VK一样 只检测一个会导致另一个变为快捷键 且无法再进入这个if
            if (vkey) {
                qDebug() << mods << vkey;
                QSettings IniSet(Path::iniFile(), QSettings::IniFormat);
                IniSet.setValue("Shortcut/teleport_modifiers", mods);
                IniSet.setValue("Shortcut/teleport_key", vkey);
                qDebug() << "writen";
                emit updateShortcut(mods, vkey, ui->lineEdit->text());
                ui->lineEdit->setText("Successful");
                QTimer::singleShot(1200, [=]() {
                    this->close();
                });
            } else
                ui->lineEdit->setText("Only Modifiers or Not Valid");

            return true;
        } else if (modifiers == Qt::NoModifier && key == Qt::Key_Escape) {
            close();
            return true;
        }

        bool isCtrl = modifiers & Qt::ControlModifier;
        bool isAlt = modifiers & Qt::AltModifier;
        bool isShift = modifiers & Qt::ShiftModifier;
        bool isWin = modifiers & Qt::MetaModifier;
        vkey = keyEvent->nativeVirtualKey();
        qDebug() << modifiers << vkey << key << keyStr;

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

        static const QSet<Qt::Key> mods_key = { Qt::Key_Control, Qt::Key_Alt, Qt::Key_Meta, Qt::Key_Shift };
        if (!mods_key.contains(key)) { //仅modifiers时，返回的key是最后一个modifier
            if (!str.isEmpty()) str += '+';
            str += keyStr.mid(4); //del Key_
        } else
            vkey = 0;

        ui->lineEdit->setText(str);
        return true;
    }
    return false;
}
