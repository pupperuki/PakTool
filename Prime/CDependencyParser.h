#ifndef CDEPENDENCYPARSER_H
#define CDEPENDENCYPARSER_H

#include <FileIO/FileIO.h>
#include "../types.h"
#include "../CFourCC.h"
#include <string>

class CDependencyParser
{
    bool ownsDependencyList;
    CTextOutStream *DependencyList;
    std::string AssetDirectory;

    void writeDependency(u32 resID, CFourCC resType);
    CInputStream* openFile(u32 resID, CFourCC resType);

public:
    CDependencyParser();
    ~CDependencyParser();

    void setDependencyList(std::string stream);
    void setDependencyList(CTextOutStream& stream);
    void setAssetDir(std::string dir);
    bool hasValidList();
    void Close();

    void parseAFSM(u32 resID);
    void parseAGSC(u32 resID);
    void parseANCS(u32 resID);
    void parseANIM(u32 resID);
    void parseATBL(u32 resID);
    void parseCINF(u32 resID);
    void parseCMDL(u32 resID);
    void parseCRSC(u32 resID);
    void parseCSKR(u32 resID);
    void parseCSNG(u32 resID);
    void parseCTWK(u32 resID);
    void parseDCLN(u32 resID);
    void parseDGRP(u32 resID);
    void parseDPSC(u32 resID);
    void parseDUMB(u32 resID);
    void parseELSC(u32 resID);
    void parseEVNT(u32 resID);
    void parseFONT(u32 resID);
    void parseFRME(u32 resID);
    void parseHINT(u32 resID);
    void parseMAPA(u32 resID);
    void parseMAPW(u32 resID);
    void parseMAPU(u32 resID);
    void parseMLVL(u32 resID);
    void parseMREA(u32 resID);
    void parsePATH(u32 resID);
    void parsePART(u32 resID);
    void parseSAVW(u32 resID);
    void parseSCAN(u32 resID);
    void parseSTRG(u32 resID);
    void parseSWHC(u32 resID);
    void parseTXTR(u32 resID);
    void parseWPSC(u32 resID);

    void parseActorParams(CInputStream *src);
    void parseLightParams(CInputStream *src);
    void parseScanParams(CInputStream *src);
    void parsePatternedInfo(CInputStream *src);
    void parseFlareDef(CInputStream *src);

    void parseAnimStructA(CInputStream *src, u32 type);
    void parseAnimStructB(CInputStream *src, u32 type);
};

#endif // CDEPENDENCYPARSER_H
