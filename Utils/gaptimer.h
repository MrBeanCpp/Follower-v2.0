#ifndef GAPTIMER_H
#define GAPTIMER_H

#include <QDateTime>
#include <QObject>
#include <QTimer>
class GapTimer : public QTimer { //用两个时间点间的Gap去匹配指定时间点（而非以点匹配点），消除因轮询间隔过长导致的错过，以及间隔过短导致的多次触发
    Q_OBJECT
public:
    explicit GapTimer(QObject* parent = nullptr);

    bool inGap(const QDateTime& dateTime); //judge time-point with Gap avoiding the precision error
    bool inGap(const QTime& time); //跨越24:00会判断错误

    // QObject interface
protected:
    void timerEvent(QTimerEvent* event) override;

private:
    QDateTime oldDateTime, newDateTime;
};

#endif // GAPTIMER_H
