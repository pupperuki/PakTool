#include "CFourCC.h"

CFourCC::CFourCC()
{
    fourCC[0] = 0;
    fourCC[1] = 0;
    fourCC[2] = 0;
    fourCC[3] = 0;
}

CFourCC::CFourCC(const char *src)
{
    *this = src;
}

CFourCC::CFourCC(long src)
{
    *this = src;
}

CFourCC::CFourCC(CInputStream& src)
{
    src.ReadBytes(&fourCC[0], 4);
}

CFourCC& CFourCC::operator=(const char *src)
{
    memcpy(&fourCC[0], src, 4);
    return *this;
}

CFourCC& CFourCC::operator=(long src)
{
    fourCC[0] = (src >> 24) & 0xFF;
    fourCC[1] = (src >> 16) & 0xFF;
    fourCC[2] = (src >>  8) & 0xFF;
    fourCC[3] = (src >>  0) & 0xFF;
    return *this;
}

bool CFourCC::operator==(const CFourCC& other) const
{
    return ((fourCC[0] == other.fourCC[0]) && (fourCC[1] == other.fourCC[1]) && (fourCC[2] == other.fourCC[2]) && (fourCC[3] == other.fourCC[3]));
}

bool CFourCC::operator==(const char *other) const
{
    return (*this == CFourCC(other));
}

bool CFourCC::operator==(const long other) const
{
    return (*this == CFourCC(other));
}

bool CFourCC::operator!=(const CFourCC& other) const
{
    return (!(*this == other));
}

bool CFourCC::operator!=(const char *other) const
{
    return (!(*this == other));
}

bool CFourCC::operator!=(const long other) const
{
    return (!(*this == other));
}

long CFourCC::asLong()
{
    return fourCC[0] << 24 | fourCC[1] << 16 | fourCC[2] << 8 | fourCC[3];
}

std::string CFourCC::asString()
{
    return std::string(fourCC, 4);
}
