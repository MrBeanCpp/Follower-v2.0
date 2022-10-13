#ifndef AUDIODEVICE_H
#define AUDIODEVICE_H
#include <QDebug>

class AudioDevice
{
public:
    AudioDevice() = default;
    AudioDevice(const QString& id):id(id), name(""){}
    AudioDevice(const QString& id, const QString& name):id(id), name(name){}
    bool isNull(void);

    static bool setDefaultOutputDevice(QString devID);
    static QList<AudioDevice> enumOutputDevice(void);
    static AudioDevice defaultOutputDevice(void);

    QString id;
    QString name;
};
QDebug operator<<(QDebug dbg, const AudioDevice& dev);
bool operator==(const AudioDevice& lhs, const AudioDevice& rhs);

#endif // AUDIODEVICE_H
