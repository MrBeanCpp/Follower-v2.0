#include "keystate.h"
#include <QDebug>
QMap<int, KeyState::Record> KeyState::lastState;
QSet<QPair<int, KeyState::State>> KeyState::lockList;
KeyState::State KeyState::state(int vKey, int releaseLimit)
{
    for (const auto& p : qAsConst(lockList)) //在clearLock之前只能检测Key一次，防止多次改变状态
        if (p.first == vKey)
            return checkReleaseLimit(vKey, p.second, releaseLimit);

    if (!lastState.contains(vKey)) lastState.insert(vKey, { Uping, QTime::currentTime() }); //default Uping
    State pastState = lastState.value(vKey).state;
    State nowState = turnToState(GetAsyncKeyState(vKey)); //
    lastState[vKey].state = nowState;

    State res;
    if (pastState == nowState) {
        res = nowState;
    } else {
        res = nowState == Uping ? Release : Press;
    }

//    if(res == Press && vKey == VK_MBUTTON)
//        qDebug() << "MB Press" << (pastState == Uping) << (nowState == Downing);

//    if(res == Release && vKey == VK_MBUTTON)
//        qDebug() << "MB Release" << (pastState == Downing) << (nowState == Uping);

    lockList.insert({ vKey, res });
    return checkReleaseLimit(vKey, res, releaseLimit);
}
bool KeyState::isState(int vKey, KeyState::State _state, int releaseLimit)
{
    return state(vKey, releaseLimit) == _state;
}

bool KeyState::isRelease(int vKey, int releaseLimit)
{
    return isState(vKey, Release, releaseLimit);
}

bool KeyState::isPress(int vKey)
{
    return isState(vKey, Press);
}

bool KeyState::isUping(int vKey)
{
    return isState(vKey, Uping);
}

bool KeyState::isDowning(int vKey)
{
    return isState(vKey, Downing);
}

void KeyState::clearLock()
{
    lockList.clear();
}

KeyState::State KeyState::turnToState(SHORT _state)
{
    return (_state & 0x8000) ? Downing : Uping;
}

KeyState::State KeyState::checkReleaseLimit(int vKey, const KeyState::State _state, int releaseLimit)
{
    if (releaseLimit <= 0) return _state;
    State res = _state;
    if (res == Release) {
        if (lastState.value(vKey).time.msecsTo(QTime::currentTime()) > releaseLimit) //超过时限后自动释放
            res = Uping;
    } else if (res == Press)
        lastState[vKey].time = QTime::currentTime();
    return res;
}
