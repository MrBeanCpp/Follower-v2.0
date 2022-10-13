#ifndef LOGFILEHANDLER_H
#define LOGFILEHANDLER_H
#include <QMessageLogContext>
#include <QMutex>
#include <QFileInfo>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
class LogFileHandler
{
public:
    LogFileHandler() = delete;
    static void startFileLogging(bool keepQtOutput = true);
    static void stopFileLogging(void);
private:
    static void outputMessage(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static QtMessageHandler qtMsgLog;
    static const QString dir;
};

#endif // LOGFILEHANDLER_H
