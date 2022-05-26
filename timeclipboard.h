#ifndef TIMECLIPBOARD_H
#define TIMECLIPBOARD_H

#include <QObject>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QDateTime>
class TimeClipboard : public QObject
{
    Q_OBJECT
public:
    explicit TimeClipboard(int maxTextLen = 64, QObject* parent = nullptr);
    QDateTime lastTextChangeTime(void);
    QString text(void);
    qint64 lastTextChangeToNow(void); //ms

private:
    QPair<QDateTime, QString> textChangeInfo { QDateTime::currentDateTime(), QString() };
};

#endif // TIMECLIPBOARD_H
