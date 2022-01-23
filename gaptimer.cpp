#include "gaptimer.h"
#include <QDebug>
GapTimer::GapTimer(QObject* parent)
    : QTimer(parent)
{
    QDateTime now = QDateTime::currentDateTime();
    oldDateTime = now;
    newDateTime = now;
}

void GapTimer::timerEvent(QTimerEvent* event)
{
    oldDateTime = newDateTime;
    newDateTime = QDateTime::currentDateTime();

    QTimer::timerEvent(event);
}

bool GapTimer::inGap(const QDateTime& dateTime)
{
    return dateTime > oldDateTime && dateTime <= newDateTime;
}

bool GapTimer::inGap(const QTime& time)
{
    return time > oldDateTime.time() && time <= newDateTime.time();
}
