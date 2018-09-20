#include <string>
#include <sstream>
#include <iomanip>
#include "StringUtil.h"

namespace StringUtil
{
    std::string getFileDirectory(std::string path)
    {
        size_t endpath = path.find_last_of("\\/");
        return path.substr(0, endpath + 1);
    }

    std::string getFileName(std::string path)
    {
        size_t endpath = path.find_last_of("\\/") + 1;
        size_t endname = path.find_last_of(".");
        return path.substr(endpath, endname - endpath);
    }

    std::string getFileNameWithExtension(std::string path)
    {
        size_t endpath = path.find_last_of("\\/");
        return path.substr(endpath + 1, path.size() - endpath);
    }

    std::string getFileWithoutExtension(std::string path)
    {
        size_t endname = path.find_last_of(".");
        return path.substr(0, endname);
    }

    std::string getFileExtension(std::string path)
    {
        size_t endname = path.find_last_of("\\/");
        return path.substr(endname + 1, path.size() - endname);
    }


    // Not convinced stringstream is the best way to do string conversions of asset IDs - don't know of a better way tho
    std::string resToStr(long assetID)
    {
        std::stringstream sstream;
        sstream << std::hex << std::setw(8) << std::setfill('0') << assetID << std::dec;
        return sstream.str();
    }

    std::string resToStr(long long assetID)
    {
        std::stringstream sstream;
        sstream << std::hex << std::setw(16) << std::setfill('0') << assetID << std::dec;
        return sstream.str();
    }

    std::string resToStr(SFileID_128 assetID)
    {
        std::stringstream sstream;
        sstream << std::hex << std::setw(16) << std::setfill('0') << assetID.a << assetID.b << std::dec;
        return sstream.str();
    }

    void ensureEndsWithSlash(std::string& str)
    {
        if (str.empty()) str = "./";

        else if ((str.back() != '/') && (str.back() != '\\'))
        {
            if (str.back() == '\0')
            {
                str.back() = '/';
                str.push_back('\0');
            }

            else
                str.push_back('/');
        }
    }

    long hash32(std::string str)
    {
        unsigned long hash = 0;

        for (u32 c = 0; c < str.size(); c++) {
            hash += str[c];
            hash *= 101;
        }

        return hash;
    }

    long long hash64(std::string str)
    {
        unsigned long long hash = 0;

        for (u32 c = 0; c < str.size(); c++) {
            hash += str[c];
            hash *= 101;
        }

        return hash;
    }

    long strToRes32(std::string str) {
        return std::stoul(str, nullptr, 16);
    }

    long long strToRes64(std::string str) {
        return std::stoull(str, nullptr, 16);
    }

    long getResID32(std::string str)
    {
        long resID;
        if (isHexString(str, 8))
            resID = strToRes32(str);
        else
            resID = hash32(getFileName(str));
        return resID;
    }

    bool isHexString(std::string str, long width)
    {
        str = getFileName(str);

        if ((str.size() == width + 2) && (str.substr(0, 2) == "0x"))
            str = str.substr(2, width);

        if (str.size() != width) return false;

        for (int c = 0; c < width; c++)
        {
            char chr = str[c];
            if (!((chr >= '0') && (chr <= '9')) &&
                !((chr >= 'a') && (chr <= 'f')) &&
                !((chr >= 'A') && (chr <= 'F')))
                return false;
        }
        return true;
    }
}

