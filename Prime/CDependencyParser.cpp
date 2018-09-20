#include "CDependencyParser.h"
#include "../StringUtil.h"

#include <iostream>

CDependencyParser::CDependencyParser()
{
    DependencyList = nullptr;
    ownsDependencyList = false;
}

CDependencyParser::~CDependencyParser()
{
    Close();
}

void CDependencyParser::setDependencyList(std::string stream)
{
    Close();
    DependencyList = new CTextOutStream(stream);
    ownsDependencyList = true;
}

void CDependencyParser::setDependencyList(CTextOutStream& stream)
{
    Close();
    DependencyList = &stream;
    ownsDependencyList = true;
}

void CDependencyParser::setAssetDir(std::string dir)
{
    AssetDirectory = dir;
    StringUtil::ensureEndsWithSlash(AssetDirectory);
}

bool CDependencyParser::hasValidList()
{
    if (DependencyList == nullptr) return false;
    return DependencyList->IsValid();
}

void CDependencyParser::Close()
{
    if ((DependencyList != nullptr) && (ownsDependencyList))
    {
        DependencyList->Close();
        delete DependencyList;
    }
    DependencyList = nullptr;
    ownsDependencyList = false;
}

void CDependencyParser::writeDependency(u32 resID, CFourCC resType)
{
    if (hasValidList())
    {
        std::string res = StringUtil::resToStr(long(resID)) + "." + resType.asString() + "\n";
        DependencyList->WriteString(res);
    }
}

CInputStream* CDependencyParser::openFile(u32 resID, CFourCC resType)
{
    std::string res = AssetDirectory + StringUtil::resToStr(long(resID)) + "." + resType.asString();
    CFileInStream *stream = new CFileInStream(res, IOUtil::BigEndian);
    if (!stream->IsValid())
    {
        std::cout << "Unable to open " << res << "\n";
        delete stream;
        return nullptr;
    }

    else return stream;
}

void CDependencyParser::parseAFSM(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "AFSM");
}

void CDependencyParser::parseAGSC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "AGSC");
}

void CDependencyParser::parseANCS(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "ANCS");

    if (src)
    {
        src->Seek(0x4, SEEK_SET);
        u32 NodeCount = src->ReadLong();

        for (u32 n = 0; n < NodeCount; n++)
        {
            src->Seek(0x2, SEEK_CUR);
            u32 flags = src->ReadLong();
            std::string name = src->ReadString();
            parseCMDL(src->ReadLong());
            parseCSKR(src->ReadLong());
            parseCINF(src->ReadLong());

            u32 AnimCount = src->ReadLong();
            for (u32 a = 0; a < AnimCount; a++)
            {
                src->Seek(0x5, SEEK_CUR);
                src->ReadString();
            }

            src->Seek(0x4, SEEK_CUR);
            u32 AnimStateCount = src->ReadLong();
            src->Seek(0x4, SEEK_CUR);
            for (u32 a = 0; a < AnimStateCount; a++)
            {
                src->Seek(0x4, SEEK_CUR);
                u32 ParmInfoCount = src->ReadLong();
                u32 AnimInfoCount = src->ReadLong();
                u32 skip = 0;

                for (u32 p = 0; p < ParmInfoCount; p++)
                {
                    u32 ParmType = src->ReadLong();
                    src->Seek(0x8, SEEK_CUR);

                    switch (ParmType)
                    {
                    case 0x0:
                    case 0x1:
                    case 0x2:
                    case 0x4:
                        src->Seek(0x8, SEEK_CUR);
                        skip += 4;
                        break;
                    case 0x3:
                        src->Seek(0x2, SEEK_CUR);
                        skip += 1;
                        break;
                    }
                }

                for (u32 ai = 0; ai < AnimInfoCount; ai++)
                {
                    src->Seek(0x4 + skip, SEEK_CUR);
                }
            }

            u32 PartCount = src->ReadLong();
            for (u32 p = 0; p < PartCount; p++)
                parsePART(src->ReadLong());

            u32 SwooshCount = src->ReadLong();
            for (u32 s = 0; s < SwooshCount; s++)
                parseSWHC(src->ReadLong());

            if (!(flags & 0x1)) src->Seek(0x4, SEEK_CUR);

            u32 ElecCount = src->ReadLong();
            for (u32 e = 0; e < ElecCount; e++)
                parseELSC(src->ReadLong());

            src->Seek(0x4, SEEK_CUR);
            u32 AnimCount2 = src->ReadLong();
            for (u32 a = 0; a < AnimCount2; a++)
            {
                src->ReadString();
                src->Seek(0x18, SEEK_CUR);
            }

            u32 EffectCount = src->ReadLong();
            for (u32 e = 0; e < EffectCount; e++)
            {
                src->ReadString();
                src->Seek(0x4, SEEK_CUR);
                src->ReadString();
                src->Seek(0x4, SEEK_CUR);
                parsePART(src->ReadLong());
                src->ReadString();
                src->Seek(0xC, SEEK_CUR);
            }

            u32 cmdl = src->ReadLong();
            if (cmdl)
                parseCMDL(cmdl);

            u32 cskr = src->ReadLong();
            if (cskr)
                parseCSKR(cskr);

            u32 UnknownCount = src->ReadLong();
            src->Seek(UnknownCount * 4, SEEK_CUR);
        }

        u16 numTables = src->ReadShort();
        if ((numTables < 2) || (numTables > 4))
            std::cout << "\rUnexpected table count in " << std::hex << resID << ".ANCS at 0x" << src->Tell() << std::dec << "\n";

        // Table 1
        u32 table1EntryCount = src->ReadLong();
        for (u32 t = 0; t < table1EntryCount; t++)
        {
            src->ReadString();
            parseAnimStructA(src, src->ReadLong());
        }

        // Table 2
        u32 table2EntryCount = src->ReadLong();
        for (u32 t = 0; t < table2EntryCount; t++)
        {
            src->Seek(0xC, SEEK_CUR);
            parseAnimStructB(src, src->ReadLong());
        }
        parseAnimStructB(src, src->ReadLong());
        u32 count = src->ReadLong();
        src->Seek(count * 0xC, SEEK_CUR);
        src->Seek(0x8, SEEK_CUR);

        // Table 3
        if (numTables >= 3)
        {
            u32 table3EntryCount = src->ReadLong();
            for (u32 t = 0; t < table3EntryCount; t++)
            {
                src->Seek(0x4, SEEK_CUR);
                parseAnimStructB(src, src->ReadLong());
            }
        }

        // Table 4
        if (numTables >= 4)
        {
            u32 table4EntryCount = src->ReadLong();
            for (u32 t = 0; t < table4EntryCount; t++)
            {
                parseANIM(src->ReadLong());
                parseEVNT(src->ReadLong());
            }
        }

        delete src;
    }

    writeDependency(resID, "ANCS");
}

void CDependencyParser::parseANIM(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "ANIM");

    if (src)
    {
        src->Seek(0x8, SEEK_SET);
        if (src->PeekLong()) parseEVNT(src->ReadLong());
        delete src;
    }

    writeDependency(resID, "ANIM");
}

void CDependencyParser::parseATBL(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "ATBL");
}

void CDependencyParser::parseCINF(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "CINF");
}

void CDependencyParser::parseCMDL(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "CMDL");

    if (src)
    {
        /*src->Seek(0x24, SEEK_SET);
        u32 section_count = src->ReadLong();
        u32 mat_set_count = src->ReadLong();
        CBlockMgrIn BlockMgr(section_count, src);
        if ((src->Tell() % 32) != 0)
            src->SeekToBoundary(32);
        BlockMgr.init();

        for (u32 m = 0; m < mat_set_count; m++)
        {
            u32 texture_count = src->ReadLong();
            for (u32 t = 0; t < texture_count; t++)
                parseTXTR(src->ReadLong());
            BlockMgr.toNextBlock();
        }

        delete src;*/
    }

    writeDependency(resID, "CMDL");
}

void CDependencyParser::parseCRSC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "CRSC");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();
            if ((check == "CODL") || (check == "DDCL") || (check == "ICDL") || (check == "GRDL") || (check == "MEDL") || (check == "WODL"))
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parseDPSC(src->ReadLong());
            }

            if ((check == "DCHR") || (check == "DEFS"))
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parsePART(src->ReadLong());
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }

    writeDependency(resID, "CRSC");
}

void CDependencyParser::parseCSKR(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "CSKR");
}

void CDependencyParser::parseCSNG(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "CSNG");
}

void CDependencyParser::parseCTWK(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "CTWK");
}

void CDependencyParser::parseDCLN(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "DCLN");
}

void CDependencyParser::parseDGRP(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "DGRP");

    if (src)
    {
        u32 DependencyCount = src->ReadLong();

        for (u32 d = 0; d < DependencyCount; d++)
        {
            CFourCC resType = src->ReadLong();
            u32 resID = src->ReadLong();

            if (resType == "AFSM") parseAFSM(resID);
            else if (resType == "AGSC") parseAGSC(resID);
            else if (resType == "ANCS") parseANCS(resID);
            else if (resType == "ANIM") parseANIM(resID);
            else if (resType == "ATBL") parseATBL(resID);
            else if (resType == "CINF") parseCINF(resID);
            else if (resType == "CMDL") parseCMDL(resID);
            else if (resType == "CRSC") parseCRSC(resID);
            else if (resType == "CSKR") parseCSKR(resID);
            else if (resType == "CSNG") parseCSNG(resID);
            else if (resType == "CTWK") parseCTWK(resID);
            else if (resType == "DCLN") parseDCLN(resID);
            else if (resType == "DGRP") parseDGRP(resID);
            else if (resType == "DPSC") parseDPSC(resID);
            else if (resType == "DUMB") parseDUMB(resID);
            else if (resType == "ELSC") parseELSC(resID);
            else if (resType == "EVNT") parseEVNT(resID);
            else if (resType == "FONT") parseFONT(resID);
            else if (resType == "FRME") parseFRME(resID);
            else if (resType == "HINT") parseHINT(resID);
            else if (resType == "MAPA") parseMAPA(resID);
            else if (resType == "MAPW") parseMAPW(resID);
            else if (resType == "MAPU") parseMAPU(resID);
            else if (resType == "MLVL") parseMLVL(resID);
            else if (resType == "MREA") parseMREA(resID);
            else if (resType == "PART") parsePART(resID);
            else if (resType == "PATH") parsePATH(resID);
            else if (resType == "SAVW") parseSAVW(resID);
            else if (resType == "SCAN") parseSCAN(resID);
            else if (resType == "STRG") parseSTRG(resID);
            else if (resType == "SWHC") parseSWHC(resID);
            else if (resType == "TXTR") parseTXTR(resID);
            else if (resType == "WPSC") parseWPSC(resID);
            // fuck that was annoying
        }

        delete src;
    }

    writeDependency(resID, "DGRP");
}

void CDependencyParser::parseDPSC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "DPSC");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();
            if (check == "1TEX")
            {
                CFourCC check2 = src->ReadLong();
                CFourCC check3 = src->ReadLong();

                if ((check2 == "CNST") && (check3 == "CNST"))
                    if (src->PeekLong())
                        parseTXTR(src->ReadLong());
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }

    writeDependency(resID, "DPSC");
}

void CDependencyParser::parseDUMB(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "DUMB");
}

void CDependencyParser::parseELSC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "ELSC");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();
            if ((check == "EPSM") || (check == "GPSM"))
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parsePART(src->ReadLong());
            }
            else if (check == "SSWH")
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parseSWHC(src->ReadLong());
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }

    writeDependency(resID, "ELSC");
}

void CDependencyParser::parseEVNT(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "EVNT");
}

void CDependencyParser::parseFONT(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "FONT");

    if (src)
    {
        src->Seek(0x22, SEEK_SET);
        src->ReadString();
        parseTXTR(src->ReadLong());

        delete src;
    }

    writeDependency(resID, "FONT");
}

void CDependencyParser::parseFRME(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "FRME");

    if (src)
    {
        src->Seek(0x10, SEEK_CUR);
        u32 WidgetCount = src->ReadLong();

        for (u32 w = 0; w < WidgetCount; w++)
        {
            CFourCC WidgetType = src->ReadLong();
            src->ReadString();
            src->ReadString();

            if ((WidgetType == "HWIG") || (WidgetType == "BWIG"))
                src->Seek(0x5B, SEEK_CUR);

            else if (WidgetType == "CAMR")
                src->Seek(0x77, SEEK_CUR);

            else if (WidgetType == "ENRG")
            {
                src->Seek(0x18, SEEK_CUR);
                parseTXTR(src->ReadLong());
                src->Seek(0x43, SEEK_CUR);
            }

            else if (WidgetType == "GRUP")
                src->Seek(0x60, SEEK_CUR);

            else if (WidgetType == "IMGP")
            {
                src->Seek(0x18, SEEK_CUR);
                u32 txtr1 = src->ReadLong();
                u32 txtr2 = src->ReadLong();

                if (txtr1 != 0xFFFFFFFF)
                    parseTXTR(txtr1);
                if (txtr2 != 0xFFFFFFFF)
                    parseTXTR(txtr2);

                src->Seek(0x9F, SEEK_CUR);
            }

            else if (WidgetType == "LITE")
                src->Seek(0x7B, SEEK_CUR);

            else if (WidgetType == "MODL")
            {
                src->Seek(0x18, SEEK_CUR);
                parseCMDL(src->ReadLong());
                src->Seek(0x4B, SEEK_CUR);
            }

            else if (WidgetType == "METR")
                src->Seek(0x65, SEEK_CUR);

            else if (WidgetType == "SLGP")
                src->Seek(0x6B, SEEK_CUR);

            else if (WidgetType == "TBGP")
                src->Seek(0x7E, SEEK_CUR);

            else if (WidgetType == "TXPN")
            {
                src->Seek(0x2C, SEEK_CUR);
                parseFONT(src->ReadLong());
                src->Seek(0x75, SEEK_CUR);
            }
        }

        delete src;
    }

    writeDependency(resID, "FRME");
}

void CDependencyParser::parseHINT(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "HINT");

    if (src)
    {
        src->Seek(0x8, SEEK_SET);
        u32 HintCount = src->ReadLong();

        for (u32 h = 0; h < HintCount; h++)
        {
            src->ReadString();
            src->Seek(0x8, SEEK_CUR);
            parseSTRG(src->ReadLong());
            src->Seek(0x14, SEEK_CUR);
            parseSTRG(src->ReadLong());
        }

        delete src;
    }

    writeDependency(resID, "HINT");
}

void CDependencyParser::parseMAPA(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "MAPA");
}

void CDependencyParser::parseMAPW(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "MAPW");

    if (src)
    {
        src->Seek(0x8, SEEK_SET);
        u32 area_count = src->ReadLong();
        for (u32 a = 0; a < area_count; a++)
            parseMAPA(src->ReadLong());

        delete src;
    }

    writeDependency(resID, "MAPW");
}

void CDependencyParser::parseMAPU(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "MAPU");
}

void CDependencyParser::parseMLVL(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "MLVL");

    if (src)
    {
        u32 deafbabe = src->ReadLong();
        u32 version = src->ReadLong();
        u32 WorldName = src->ReadLong();
        u32 WorldSave = src->ReadLong();
        u32 WorldSky = src->ReadLong();

        u32 MemRelayCount = src->ReadLong();
        src->Seek(11 * MemRelayCount, SEEK_CUR); // skipping memory relays

        u32 AreaCount = src->ReadLong();
        u32 unknown = src->ReadLong();
        for (u32 a = 0; a < AreaCount; a++)
        {
            std::cout << "\rParsing area " << std::dec << a << "/" << AreaCount;

            u32 AreaName = src->ReadLong();
            src->Seek(0x48, SEEK_CUR); // skipping transform matrix and AABB
            u32 Area = src->ReadLong();
            src->Seek(0x4, SEEK_CUR);

            u32 AttachedIDCount = src->ReadLong();
            src->Seek(0x2 * AttachedIDCount + 4, SEEK_CUR);
            u32 DependencyCount = src->ReadLong();
            src->Seek(0x8 * DependencyCount, SEEK_CUR);
            u32 LayerOffsetCount = src->ReadLong();
            src->Seek(0x4 * LayerOffsetCount, SEEK_CUR);
            u32 AttachedAreaCount = src->ReadLong();
            for (u32 aa = 0; aa < AttachedAreaCount; aa++)
            {
                u32 check = src->ReadLong();
                if (check == 1) src->Seek(0x3C, SEEK_CUR);
                else src->Seek(0x34, SEEK_CUR);
            }

            parseMREA(Area);
            parseSTRG(AreaName);
        }

        parseCMDL(WorldSky);
        parseSTRG(WorldName);
        parseSAVW(WorldSave);
        parseMAPW(src->ReadLong());

        delete src;
    }

    writeDependency(resID, "MLVL");
}

void CDependencyParser::parseMREA(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "MREA");

    if (src)
    {
        /*src->Seek(0x3C, SEEK_SET);
        u32 SectionCount = src->ReadLong();
        u32 GeometrySectionID = src->ReadLong();
        u32 ScriptSectionID = src->ReadLong();
        src->Seek(0x10, SEEK_CUR);
        u32 PathSectionID = src->ReadLong();
        src->Seek(0x4, SEEK_CUR);

        CBlockMgrIn BlockMgr(SectionCount, src);
        if ((src->Tell() % 32) != 0)
            src->SeekToBoundary(32);
        BlockMgr.init();

        // textures
        BlockMgr.toBlock(GeometrySectionID);
        u32 TextureCount = src->ReadLong();
        for (u32 t = 0; t < TextureCount; t++)
        {
            parseTXTR(src->ReadLong());
        }

        // scly
        // oh boy here we go
        BlockMgr.toBlock(ScriptSectionID);
        src->Seek(0x8, SEEK_CUR);
        u32 LayerCount = src->ReadLong();

        std::vector<u32> LayerSizes(LayerCount);
        for (u32 l = 0; l < LayerCount; l++)
            LayerSizes[l] = src->ReadLong();

        for (u32 l = 0; l < LayerCount; l++)
        {
            u32 LayerEnd = src->Tell() + LayerSizes[l];

            src->Seek(0x1, SEEK_CUR);
            u32 ObjectCount = src->ReadLong();

            for (u32 o = 0; o < ObjectCount; o++)
            {
                u8 ObjectType = src->ReadByte();
                u32 ObjectSize = src->ReadLong();
                u32 ObjectEnd = src->Tell() + ObjectSize;
                src->Seek(0x4, SEEK_CUR); // skipping object ID
                u32 ConnectionCount = src->ReadLong();
                src->Seek(0xC * ConnectionCount, SEEK_CUR);
                src->Seek(0x4, SEEK_CUR);
                if (ObjectType != 0x4E) src->ReadString(); // every object except AreaAttributes (0x4E) starts with a string property

                u32 ObjectStart = src->Tell();
                switch (ObjectType)
                {

                case 0x0: // Actor
                    src->Seek(0xC4, SEEK_CUR);
                    parseCMDL(src->ReadLong());
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0xD4, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x3: // DoorArea
                    src->Seek(0x24, SEEK_CUR);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x30, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x7: // Effect
                    src->Seek(0x24, SEEK_CUR);
                    parsePART(src->ReadLong());
                    parseELSC(src->ReadLong());
                    break;

                case 0x8: // Platform
                    src->Seek(0x3C, SEEK_CUR);
                    parseCMDL(src->ReadLong());
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x4C, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0xCE, SEEK_SET);
                    parseDCLN(src->ReadLong());
                    break;

                case 0xE: // NewIntroBoss
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1FE, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    break;

                case 0x11: // Pickup
                    src->Seek(ObjectStart + 0x54, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseANCS(src->ReadLong());
                    src->Seek(0x8, SEEK_CUR);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0xE6, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x16: // Beetle
                    src->Seek(0x28, SEEK_CUR);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x2F2, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    break;

                case 0x17: // HUD Memo
                    src->Seek(ObjectStart + 0x9, SEEK_SET);
                    parseSTRG(src->ReadLong());
                    break;

                case 0x18: // Camera Filter Keyframe
                    src->Seek(ObjectStart + 0x29, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    break;

                case 0x1A: // Damageable Trigger
                    src->Seek(ObjectStart + 0x9C, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    break;

                case 0x21: // Warwasp
                    src->Seek(ObjectStart + 0x28, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1FB, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x213, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    break;

                case 0x24: // Space Pirate
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1FF, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x233, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    break;

                case 0x25: // Flying Pirate
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1EE, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x20A, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x222, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x22E, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x26A, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    break;

                case 0x26: // Elite Pirate
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x202, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x20A, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x287, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x293, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x29B, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    src->Seek(ObjectStart + 0x2B7, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x2EB, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x303, SEEK_SET);
                    parseELSC(src->ReadLong());
                    break;

                case 0x27: // Metroid Beta
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x2EE, SEEK_SET);
                    parsePART(src->ReadLong());
                    parseSWHC(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    break;

                case 0x28: // Chozo Ghost
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1F2, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x20A, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x2A2, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x2D: // Blood Flower
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E2, SEEK_SET);
                    parsePART(src->ReadLong());
                    parseWPSC(src->ReadLong());
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x22A, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(0x4, SEEK_CUR);
                    parsePART(src->ReadLong());
                    break;

                case 0x2E: // Flicker Bat
                case 0x3D: // Zoomer
                case 0x3F: // Ripper
                    src->Seek(ObjectStart + 0x28, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x31: // Puddle Spore
                    src->Seek(ObjectStart + 0x28, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E7, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x1FF, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    break;

                case 0x36: // Fire Flea
                case 0x37: // Metaree Alpha
                case 0x3B: // Spank Weed
                case 0x54: // Jelly Zap
                case 0x5C: // Flaahgra Tentacle
                case 0x5F: // Thardus Rock Projectile
                case 0x6F: // Oculus
                case 0x70: // Geemer
                case 0x7A: // Tryclops
                case 0x7C: // Seedling
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x42: // Point Of Interest
                    src->Seek(ObjectStart + 0x19, SEEK_SET);
                    parseScanParams(src);
                    break;

                case 0x44: // Metroid Alpha
                    src->Seek(ObjectStart + 0x28, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x2E6, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(0x8, SEEK_CUR);
                    parseANCS(src->ReadLong());
                    src->Seek(0x8, SEEK_CUR);
                    parseANCS(src->ReadLong());
                    src->Seek(0x8, SEEK_CUR);
                    parseANCS(src->ReadLong());
                    src->Seek(0x8, SEEK_CUR);
                    break;

                case 0x45: // Debris Extended
                    src->Seek(ObjectStart + 0x8C, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x10D, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x123, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x139, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x4A: // Electro Magnetic Pulse
                    src->Seek(ObjectStart + 0x35, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x4B: // Ice Sheegoth
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x356, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x376, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x392, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parseELSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x3D2, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    src->Seek(ObjectStart + 0x3DA, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x4C: // Player Actor
                    src->Seek(ObjectStart + 0xC4, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0xD4, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x4D: // Flaahgra
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x266, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x27E, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x296, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x2AE, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x337, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x343, SEEK_SET);
                    parseDGRP(src->ReadLong());
                    break;

                case 0x4F: // Fish Cloud
                    src->Seek(ObjectStart + 0x25, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseANCS(src->ReadLong());
                    break;

                case 0x51: // Visor Flare
                    src->Seek(ObjectStart + 0x22, SEEK_SET);
                    parseFlareDef(src);
                    src->Seek(ObjectStart + 0x42, SEEK_SET);
                    parseFlareDef(src);
                    src->Seek(ObjectStart + 0x62, SEEK_SET);
                    parseFlareDef(src);
                    src->Seek(ObjectStart + 0x82, SEEK_SET);
                    parseFlareDef(src);
                    src->Seek(ObjectStart + 0xA2, SEEK_SET);
                    parseFlareDef(src);
                    break;

                case 0x53: // VisorGoo
                    src->Seek(ObjectStart + 0xC, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x58: // Thardus
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E4, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parseAFSM(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x25C, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    src->Seek(ObjectStart + 0x264, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x5A: // Wall Crawler Swarm
                    src->Seek(ObjectStart + 0x25, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0xA6, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0xBA, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    break;

                case 0x62: // World Teleporter
                    src->Seek(ObjectStart + 0x9, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x21, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    src->Seek(ObjectStart + 0x31, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    src->Seek(ObjectStart + 0x4F, SEEK_SET);
                    parseFONT(src->ReadLong());
                    parseSTRG(src->ReadLong());
                    break;

                case 0x64: // Gun Turret
                    src->Seek(ObjectStart + 0x40, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x4C, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x19A, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x1CE, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    break;

                case 0x66: // Babygoth
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1EA, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x216, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x31A, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseCSKR(src->ReadLong());
                    src->Seek(ObjectStart + 0x32A, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x352, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    src->Seek(ObjectStart + 0x35A, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x67: // Eyeball
                    src->Seek(ObjectStart + 0x28, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x169, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1EE, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x206, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    parseTXTR(src->ReadLong());
                    break;

                case 0x6D: // Snake Weed Swarm
                    src->Seek(ObjectStart + 0x19, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x25, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x6E: // ActorContraption
                    src->Seek(ObjectStart + 0xC4, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0xD0, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x14D, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x72: // Atomic Alpha
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E2, SEEK_SET);
                    parseWPSC(src->ReadLong());
                    parseCMDL(src->ReadLong());
                    break;

                case 0x75: // Ambient AI
                    src->Seek(ObjectStart + 0xC0, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0xCC, SEEK_SET);
                    parseActorParams(src);
                    break;

                case 0x77: // Atomic Beta
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E2, SEEK_SET);
                    parseELSC(src->ReadLong());
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x1FE, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x79: // Puffer
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E6, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x1FE, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    break;

                case 0x7F: // Burrower
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E2, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parseWPSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x202, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(0x4, SEEK_CUR);
                    parsePART(src->ReadLong());
                    break;

                case 0x83: // Metroid Prime Stage 2
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E2, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x1FA, SEEK_SET);
                    parseELSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x202, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;

                case 0x86: // Omega Pirate
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x202, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x20A, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x287, SEEK_SET);
                    parseANCS(src->ReadLong());
                    src->Seek(ObjectStart + 0x293, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x29B, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    src->Seek(ObjectStart + 0x2B7, SEEK_SET);
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x2EB, SEEK_SET);
                    parsePART(src->ReadLong());
                    src->Seek(ObjectStart + 0x303, SEEK_SET);
                    parseELSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x30D, SEEK_SET);
                    parseCMDL(src->ReadLong());
                    parseCSKR(src->ReadLong());
                    parseCINF(src->ReadLong());
                    break;

                case 0x88: // Phazon Healing Nodule
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x1E3, SEEK_SET);
                    parseELSC(src->ReadLong());
                    break;

                case 0x8B: // Beam Energy Ball
                    src->Seek(ObjectStart + 0x24, SEEK_SET);
                    parsePatternedInfo(src);
                    src->Seek(ObjectStart + 0x165, SEEK_SET);
                    parseActorParams(src);
                    src->Seek(ObjectStart + 0x202, SEEK_SET);
                    parseTXTR(src->ReadLong());
                    src->Seek(ObjectStart + 0x20A, SEEK_SET);
                    parsePART(src->ReadLong());
                    parseELSC(src->ReadLong());
                    src->Seek(ObjectStart + 0x21E, SEEK_SET);
                    parsePART(src->ReadLong());
                    break;
                }
                src->Seek(ObjectEnd, SEEK_SET);
            }

            src->Seek(LayerEnd, SEEK_SET);
        }

        // path
        BlockMgr.toBlock(PathSectionID);
        parsePATH(src->ReadLong());

        delete src;*/
    }

    writeDependency(resID, "MREA");
}


void CDependencyParser::parsePATH(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "PATH");
}

void CDependencyParser::parsePART(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "PART");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();
            if (check == "TEXR")
            {
                CFourCC check2 = src->ReadLong();
                CFourCC check3 = src->ReadLong();
                if ((check2 == "CNST") && (check3 == "CNST"))
                    if (src->PeekLong())
                        parseTXTR(src->ReadLong());
            }
            else if (check == "PMDL")
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parseCMDL(src->ReadLong());
            }
            else if (check == "KSSM")
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                {
                    src->Seek(0x10, SEEK_CUR);
                    if (src->ReadLong())
                    {
                        src->Seek(0x4, SEEK_CUR);
                        u32 partcount = src->ReadLong();
                        for (u32 p = 0; p < partcount; p++)
                        {
                            parsePART(src->ReadLong());
                            src->Seek(0xC, SEEK_CUR);
                        }
                    }
                }
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }

    writeDependency(resID, "PART");
}

void CDependencyParser::parseSAVW(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "SAVW");
}

void CDependencyParser::parseSCAN(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "SCAN");

    if (src)
    {
        src->Seek(0x8, SEEK_SET);
        u32 FRME = src->ReadLong();
        u32 STRG = src->ReadLong();
        src->Seek(0x19, SEEK_SET);

        if (FRME != 0xFFFFFFFF)
            parseFRME(FRME);
        if (STRG != 0xFFFFFFFF)
            parseSTRG(STRG);

        for (u32 e = 0; e < 4; e++)
        {
            u32 TXTR = src->ReadLong();
            src->Seek(0x18, SEEK_CUR);

            if (TXTR != 0xFFFFFFFF)
                parseTXTR(TXTR);
        }

        delete src;
    }

    writeDependency(resID, "SCAN");
}

void CDependencyParser::parseSTRG(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "STRG");
}

void CDependencyParser::parseSWHC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "SWHC");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();
            if (check == "TEXR")
            {
                CFourCC check2 = src->ReadLong();
                CFourCC check3 = src->ReadLong();
                if ((check2 == "CNST") && (check3 == "CNST"))
                    if (src->PeekLong())
                        parseTXTR(src->ReadLong());
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }
    writeDependency(resID, "SWHC");
}

void CDependencyParser::parseTXTR(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    writeDependency(resID, "TXTR");
}

void CDependencyParser::parseWPSC(u32 resID)
{
    if (resID == 0xFFFFFFFF) return;
    CInputStream *src = openFile(resID, "WPSC");

    if (src)
    {
        while (true)
        {
            CFourCC check = src->ReadLong();

            if ((check == "APS2") || (check == "APSM"))
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parsePART(src->ReadLong());
            }

            else if (check == "COLR")
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parseCRSC(src->ReadLong());
            }

            else if (check == "OHEF")
            {
                CFourCC check2 = src->ReadLong();
                if (check2 == "CNST")
                    if (src->PeekLong())
                        parseCMDL(src->ReadLong());
            }

            if (src->EoF()) break;
            src->Seek(-0x3, SEEK_CUR);
        }

        delete src;
    }

    writeDependency(resID, "WPSC");
}

// Each of the following functions adds 4 bytes in order to skip the property count
void CDependencyParser::parseActorParams(CInputStream *src)
{
    u32 start = src->Tell() + 4;
    src->Seek(start + 0x0, SEEK_SET);
    parseLightParams(src);
    src->Seek(start + 0x47, SEEK_SET);
    parseScanParams(src);
    src->Seek(start + 0x4F, SEEK_SET);
    parseCMDL(src->ReadLong());
}

void CDependencyParser::parseLightParams(CInputStream *src)
{
    // No dependencies that I know of - putting this here in case one is found later
}

void CDependencyParser::parseScanParams(CInputStream *src)
{
    u32 start = src->Tell() + 4;
    src->Seek(start + 0x0, SEEK_SET);
    parseSCAN(src->ReadLong());
}

void CDependencyParser::parsePatternedInfo(CInputStream *src)
{
    u32 start = src->Tell() + 4;
    src->Seek(start + 0xF4, SEEK_SET);
    parseANCS(src->ReadLong());
    src->Seek(start + 0x101, SEEK_SET);
    parseAFSM(src->ReadLong());
    src->Seek(start + 0x121, SEEK_SET);
    parsePART(src->ReadLong());
    src->Seek(start + 0x135, SEEK_SET);
    parsePART(src->ReadLong());
}

void CDependencyParser::parseFlareDef(CInputStream *src)
{
    u32 start = src->Tell() + 4;
    src->Seek(start + 0x0, SEEK_SET);
    parseTXTR(src->ReadLong());
}

void CDependencyParser::parseAnimStructA(CInputStream *src, u32 type)
{
    switch (type)
    {
    case 0:
        parseANIM(src->ReadLong());
        src->Seek(0x4, SEEK_CUR);
        src->ReadString();
        src->Seek(0x8, SEEK_CUR);
        break;
    case 3:
    case 4:
    {
        u32 numSubStructs = src->ReadLong();
        for (u32 s = 0; s < numSubStructs; s++)
        {
            u32 _type = src->ReadLong();
            parseAnimStructA(src, _type);
            if (type == 3) src->Seek(0x4, SEEK_CUR);
        }
        break;
    }
    default:
        std::cout << "\rERROR: Unknown AnimStructA of type " << type << " encountered\n";
    }
}

void CDependencyParser::parseAnimStructB(CInputStream *src, u32 type)
{
    switch (type)
    {
    case 0: {
        u32 _type = src->ReadLong();
        parseAnimStructA(src, _type);
        break;
    }
    case 1:
    case 2:
        src->Seek(0xE, SEEK_CUR);
        break;
    case 3:
        break;
    default:
        std::cout << "\rERROR: Unknown AnimStructB of type " << type << " encountered\n";
    }
}
