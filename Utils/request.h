#ifndef REQUEST_H
#define REQUEST_H

#include <QNetworkAccessManager>
#include <functional>
#include <QString>
#include <QObject>
class Request : public QObject {
    Q_OBJECT
public:
    Request(QObject* parent = nullptr);
    void translate(const QString& text, std::function<void(const QString&)> todo, int timeout = 2000, const QString& from = "auto", const QString& to = "en");

private:
    QNetworkAccessManager* Manager = nullptr;
};

#endif // REQUEST_H
