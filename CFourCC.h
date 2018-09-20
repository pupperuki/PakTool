#ifndef CFOURCC_H
#define CFOURCC_H

#include <FileIO/CInputStream.h>
#include <string>

class CFourCC
{
    char fourCC[4];
public:
    CFourCC();
    CFourCC(const char *src);
    CFourCC(long src);
    CFourCC(CInputStream& src);

    CFourCC& operator=(const char *src);
    CFourCC& operator=(long src);
    bool operator==(const CFourCC& other) const;
    bool operator==(const char *other) const;
    bool operator==(const long other) const;
    bool operator!=(const CFourCC& other) const;
    bool operator!=(const char *other) const;
    bool operator!=(const long other) const;

    long asLong();
    std::string asString();
};

#endif // CFOURCC_H
