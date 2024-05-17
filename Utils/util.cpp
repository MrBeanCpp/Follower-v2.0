#include "util.h"

bool Util::maybePath(const QString& str)
{
    if (str.size() < 2) return false;
    return str[0].isLetter() && str[1] == ':';
}
