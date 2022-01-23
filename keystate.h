#ifndef KEYSTATE_H
#define KEYSTATE_H

#include <QMap>
#include <QPair>
#include <QSet>
#include <QTime>
#include <set>
#include <windows.h>

class KeyState
{
public:
    enum State {
        Downing,
        Uping,
        Press,
        Release
    };
    KeyState() = delete;
    static State state(int vKey, int releaseLimit = -1);
    static bool isState(int vKey, State _state, int releaseLimit = -1);
    static bool isRelease(int vKey, int releaseLimit = -1); //释放时长：Downing时间>releaseLimit自动认为释放（releaseLimit<=0无效）
    static bool isPress(int vKey);
    static bool isUping(int vKey);
    static bool isDowning(int vKey);
    static void clearLock(void);

private:
    struct Record { //C++居然有内部类 WTF
        KeyState::State state;
        QTime time;
    };

private:
    static State turnToState(SHORT _state);
    static State checkReleaseLimit(int vKey, const State _state, int releaseLimit);

private:
    static QMap<int, Record> lastState; //Uping | Downing
    static QSet<QPair<int, KeyState::State>> lockList;
};

#endif // KEYSTATE_H
