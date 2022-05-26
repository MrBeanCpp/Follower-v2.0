#include "timeclipboard.h"
#include <QFile>

TimeClipboard::TimeClipboard(int maxTextLen, QObject* parent)
    : QObject(parent)
{
    connect(
        qApp->clipboard(), &QClipboard::dataChanged, this, [=]() {
            const QMimeData* data = qApp->clipboard()->mimeData();
            if (data->hasText() && !data->hasUrls()) { //排除复制文件
                const QString text = data->text();
                if (text.length() < maxTextLen && !QFile::exists(text)) //排除复制路径
                    textChangeInfo = qMakePair(QDateTime::currentDateTime(), text); //记录时间
            }
        },
        Qt::QueuedConnection);
}

QDateTime TimeClipboard::lastTextChangeTime()
{
    return textChangeInfo.first;
}

QString TimeClipboard::text()
{
    return textChangeInfo.second;
}

qint64 TimeClipboard::lastTextChangeToNow()
{
    return lastTextChangeTime().msecsTo(QDateTime::currentDateTime());
}
