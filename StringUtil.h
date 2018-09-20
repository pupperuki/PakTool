#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <string>
#include "TropicalFreeze/SFileID_128.h"

namespace StringUtil
{
    std::string getFileDirectory(std::string path);
    std::string getFileName(std::string path);
    std::string getFileNameWithExtension(std::string path);
    std::string getFileWithoutExtension(std::string path);
    std::string getFileExtension(std::string path);
    std::string resToStr(long ID);
    std::string resToStr(long long ID);
    std::string resToStr(SFileID_128 ID);
    void ensureEndsWithSlash(std::string& str);
    long      hash32(std::string str);
    long long hash64(std::string str);
    long      strToRes32(std::string str);
    long long strToRes64(std::string str);
    long      getResID32(std::string str);
    bool isHexString(std::string str, long width);
}

#endif // STRINGUTIL_H
