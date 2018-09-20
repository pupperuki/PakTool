#include <iostream>
#include <boost/filesystem.hpp>
#include "PakList.h"
#include "StringUtil.h"
#include "TropicalFreeze/CTropicalFreezePak.h"
#include "Prime/CDependencyParser.h"
#include "PakEnum.h"
#include "Prime/CPrimePak.h"
#include "Corruption/CCorruptionPak.h"

enum EPakToolMode
{
    Unpack, Repack, ListDump
};

EWhichGame ArgToEnum(char *arg)
{
    std::string str(arg);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    if (str == "MPDEMO")       return MP1Demo;
    else if (str == "MP1DEMO") return MP1Demo;

    else if (str == "MP")           return MP1;
    else if (str == "MP1")          return MP1;
    else if (str == "METROIDPRIME") return MP1;
    else if (str == "PRIME")        return MP1;
    else if (str == "PRIME1")       return MP1;

    else if (str == "MP2DEMO")    return MP2Demo;
    else if (str == "ECHOESDEMO") return MP2Demo;

    else if (str == "MP2")                 return MP2;
    else if (str == "MP2E")                return MP2;
    else if (str == "MP2:E")               return MP2;
    else if (str == "METROIDPRIME2")       return MP2;
    else if (str == "PRIME2")              return MP2;
    else if (str == "ECHOES")              return MP2;
    else if (str == "METROIDPRIME2ECHOES") return MP2;

    else if (str == "MP3DEMO")         return MP3Proto;
    else if (str == "MP3PROTO")        return MP3Proto;
    else if (str == "MP3PROTOTYPE")    return MP3Proto;
    else if (str == "CORRUPTIONPROTO") return MP3Proto;
    else if (str == "CORRUPTIONDEMO")  return MP3Proto;

    else if (str == "MP3")                     return MP3;
    else if (str == "MP3C")                    return MP3;
    else if (str == "MP3:C")                   return MP3;
    else if (str == "METROIDPRIME3")           return MP3;
    else if (str == "PRIME3")                  return MP3;
    else if (str == "CORRUPTION")              return MP3;
    else if (str == "METROIDPRIME3CORRUPTION") return MP3;

    else if (str == "MPT")                 return Trilogy;
    else if (str == "MP:T")                return Trilogy;
    else if (str == "TRILOGY")             return Trilogy;
    else if (str == "MPTRILOGY")           return Trilogy;
    else if (str == "METROIDPRIMETRILOGY") return Trilogy;

    else if (str == "DKCR")                     return DKCR;
    else if (str == "RETURNS")                  return DKCR;
    else if (str == "DONKEYKONGCOUNTRYRETURNS") return DKCR;

    else if (str == "DKCTF")                           return DKCTF;
    else if (str == "TF")                              return DKCTF;
    else if (str == "TROPICALFREEZE")                  return DKCTF;
    else if (str == "DONKEYKONGCOUNTRYTROPICALFREEZE") return DKCTF;

    else return InvalidGame;
}

void Usage()
{
    std::cout << "Usage: PakTool [mode] [options]\n"
              << "\n"
              << "Supported modes:\n"
              << "-x: Extract\n"
              << "-r: Repack\n"
              << "-d: Dump file list\n"
              << "\n"
              << "Specify a mode for info on options.\n"
              << "\n"
              << "See the readme for more details.\n";
}

void ExtractionUsage()
{
    std::cout << "Usage: PakTool -x [INPUT.pak]\n"
              << "  Alt: PakTool -xg [GAME]\n"
              << "\n"
              << "Optional: -o [dir]: set output directory\n"
              << "          -e: handle embedded compression\n"
              << "\n"
              << "See the readme for more details.\n";
}

void RepackUsage()
{
    std::cout << "Usage: PakTool -r [GAME] [INPUT FOLDER] [OUTPUT.pak] [LIST.txt]\n"
              << "  Alt: PakTool -ra [GAME] [FOLDER/OUTPUT/LIST NAME]\n"
              << "\n"
              << "Optional: -e: handle embedded compression\n"
              << "          -n: disable compression\n"
              << "\n"
              << "The two optional arguments can't be used simultaneously.\n"
              << "\n"
              << "See the readme for more details.\n";
}

void ListDumpUsage()
{
    std::cout << "Usage: PakTool -d [INPUT.pak]\n"
              << "  Alt: PakTool -dg [GAME]\n"
              << "\n"
              << "Optional: -o [dir]: set output directory\n"
              << "\n"
              << "See the readme for more details.\n";
}

void SupportedVersions()
{
    std::cout << "The pak formats from the following games are supported:\n"
              << "* Metroid Prime\n"
              << "* Metroid Prime 2: Echoes\n"
              << "* Metroid Prime 3: Corruption\n"
              << "* Donkey Kong Country Returns\n"
              << "* Donkey Kong Country: Tropical Freeze\n"
              << "* All known demos and prototypes of all of the above games\n";
}

std::vector<std::string> GlobalPakQueue;

void AddPakList(EWhichGame game)
{
    switch (game)
    {

        case MP1Demo:
            GlobalPakQueue.reserve(sizeof(PrimeDemoPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(PrimeDemoPakList[i]);
            break;

        case MP1:
            GlobalPakQueue.reserve(sizeof(PrimePakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(PrimePakList[i]);
            break;

        case MP2Demo:
            GlobalPakQueue.reserve(sizeof(EchoesDemoPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(EchoesDemoPakList[i]);
            break;

        case MP2:
            GlobalPakQueue.reserve(sizeof(EchoesPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(EchoesPakList[i]);
            break;

        case MP3Proto:
            GlobalPakQueue.reserve(sizeof(CorruptionProtoPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(CorruptionProtoPakList[i]);
            break;

        case MP3:
            GlobalPakQueue.reserve(sizeof(CorruptionPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(CorruptionPakList[i]);
            break;

        case Trilogy:
            GlobalPakQueue.reserve(sizeof(TrilogyPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(TrilogyPakList[i]);
            break;

        case DKCR:
            GlobalPakQueue.reserve(sizeof(DonkeyKongCountryReturnsPakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(DonkeyKongCountryReturnsPakList[i]);
            break;

        case DKCTF:
            GlobalPakQueue.reserve(sizeof(TropicalFreezePakList) / sizeof(std::string));
            for (u32 i = 0; i < GlobalPakQueue.capacity(); i++)
                GlobalPakQueue.push_back(TropicalFreezePakList[i]);
            break;
    }
}

void SupportedPakLists()
{
    std::cout << "MP1........Metroid Prime\n"
              << "MP2........Metroid Prime 2: Echoes\n"
              << "MP3........Metroid Prime 3: Corruption\n"
              << "DKCR.......Donkey Kong Country Returns\n"
              << "DKCTF......Donkey Kong Country: Tropical Freeze\n"
              << "MP1demo....Metroid Prime Kiosk Demo\n"
              << "MP2demo....Metroid Prime 2 Demo Disc\n"
              << "MP3proto...Metroid Prime 3 E3 Prototype\n"
              << "Trilogy....Metroid Prime: Trilogy\n";
}

void ExtractQueue(std::string directory, bool handleEmbeddedCompression, EPakToolMode mode)
{
    // Could probably cut down on redundant code by creating a CPak class
    // and having CPrimePak/CCorruptionPak/CTropicalFreezePak inherit from it
    for (u32 p = 0; p < GlobalPakQueue.size(); p++)
    {
        std::string PakName = StringUtil::getFileName(GlobalPakQueue[p]);
        std::string PakDir = StringUtil::getFileDirectory(GlobalPakQueue[p]);
        std::string PakOutputDir;

        std::cout << GlobalPakQueue[p] << "\n";

        CFileInStream pak(GlobalPakQueue[p], IOUtil::BigEndian);

        if (!pak.IsValid())
        {
            std::cout << "Error: Unable to open " << GlobalPakQueue[p] << "\n\n";
            continue;
        }

        // Get output directory
        if (!directory.empty())
        {
            StringUtil::ensureEndsWithSlash(directory);
            if (GlobalPakQueue.size() == 1)
                PakOutputDir = directory;
            else
            {
                PakOutputDir = directory + PakDir;
                if (mode == Unpack) PakOutputDir += PakName;
            }
        }
        else
        {
            PakOutputDir = PakDir;
            if (mode == Unpack) PakOutputDir += PakName + "-pak";
        }

        StringUtil::ensureEndsWithSlash(PakOutputDir);

        // Check version and unpack
        EPakVersion version = EPakVersion(pak.PeekLong());

        switch (version)
        {
        case PrimePak:
        {
            std::cout << "Version: Metroid Prime\n";

            CPrimePak MPPak(pak);
            boost::filesystem::create_directories(PakOutputDir);

            if (mode == Unpack)
            {
                if (handleEmbeddedCompression) std::cout << "Embedded compression handling enabled\n";
                else std::cout << "Embedded compression handling disabled\n";

                std::cout << "Pak contains " << std::dec << MPPak.GetResourceCount() << " files, " << MPPak.GetUniqueResourceCount() << " unique\n";
                MPPak.Extract(PakOutputDir, handleEmbeddedCompression);
            }
            else if (mode == ListDump)
            {
                std::cout << "Pak contains " << std::dec << MPPak.GetNamedResourceCount() << " filenames\n";
                MPPak.DumpList(PakOutputDir + PakName + "-pak.txt");
            }

            std::cout << "\n";
            break;
        }

        case CorruptionPak:
        {
            std::cout << "Version: Metroid Prime 3: Corruption\n";

            CCorruptionPak MP3Pak(pak);
            boost::filesystem::create_directories(PakOutputDir);

            if (mode == Unpack)
            {
                if (handleEmbeddedCompression) std::cout << "Embedded compression handling enabled\n";
                else std::cout << "Embedded compression handling disabled\n";

                std::cout << "Pak contains " << std::dec << MP3Pak.GetResourceCount() << " files, " << MP3Pak.GetUniqueResourceCount() << " unique\n";
                MP3Pak.Extract(PakOutputDir, handleEmbeddedCompression);
            }
            else if (mode == ListDump)
            {
                std::cout << "Pak contains " << std::dec << MP3Pak.GetNamedResourceCount() << " filenames\n";
                MP3Pak.DumpList(PakOutputDir + PakName + "-pak.txt");
            }

            std::cout << "\n";
            break;
        }

        case TropicalFreezePak:
        {
            pak.Seek(0x14, SEEK_SET);
            CFourCC check(pak);
            pak.Seek(0x0, SEEK_SET);

            if (check == "PACK")
            {
                std::cout << "Version: Donkey Kong Country: Tropical Freeze\n";

                CTropicalFreezePak TFPak(pak);
                boost::filesystem::create_directories(PakOutputDir);

                if (mode == Unpack)
                {
                    if (handleEmbeddedCompression) std::cout << "Embedded compression handling enabled\n";
                    else std::cout << "Embedded compression handling disabled\n";

                    std::cout << "Pak contains " << std::dec << TFPak.GetDirEntryCount() << " files\n";
                    TFPak.Extract(PakOutputDir, handleEmbeddedCompression);
                }
                else if (mode == ListDump)
                {
                    std::cout << "Pak contains " << std::dec << TFPak.GetStringEntryCount() << " filenames\n";
                    TFPak.DumpList(PakOutputDir + PakName + "-pak.txt");
                }
                std::cout << "\n";

                break; // Proceed to default case if check is not "PACK"
            }
        }

        default:
            std::cout << "Version: Invalid\n";
            std::cout << "This file is either not a valid pak or an unsupported version.\n\n";
            continue;
        }
    }
}

void RepackPak(EWhichGame game, ECompressMode mode, std::string FileDir, std::string OutputPak, std::string FileList)
{
    std::cout << "Repacking for ";
    if      (game == MP1)      std::cout << "Metroid Prime\n";
    else if (game == MP1Demo)  std::cout << "Metroid Prime Demo\n";
    else if (game == MP2)      std::cout << "Metroid Prime 2: Echoes\n";
    else if (game == MP2Demo)  std::cout << "Metroid Prime 2 Demo\n";
    else if (game == MP3)      std::cout << "Metroid Prime 3: Corruption\n";
    else if (game == MP3Proto) std::cout << "Metroid Prime 3 E3 Prototype\n";
    else if (game == DKCR)     std::cout << "Donkey Kong Country Returns\n";
    else if (game == DKCTF)
    {
        std::cout << "Donkey Kong Country: Tropical Freeze\n"
                  << "Repacking is not supported for Tropical Freeze. Sorry!\n";
        return;
    }
    else if (game == Trilogy)
    {
        std::cout << "Metroid Prime: Trilogy\n"
                  << "Trilogy uses different pak formats for each game. Please specify\n"
                  << "the game you want to repack for.\n";
        return;
    }
    std::cout << "Compression ";
    if (mode == NoCompression) std::cout << "disabled\n";
    else if (mode == Compress) std::cout << "enabled\n";
    else if (mode == CompressEmbedded) std::cout << "enabled, with embedded compression handling\n";

    FILE *f = fopen(FileList.c_str(), "r");
    if (!f)
    {
        std::cout << "Error: Unable to open list file!\n";
        return;
    }

    if ((game == MP1) || (game == MP2) || (game == MP1Demo) || (game == MP2Demo) || (game == MP3Proto))
    {
        CPrimePak pak;
        pak.BuildTOC(f, game);
        std::cout << "Pak will contain " << pak.GetResourceCount() << " files\n";

        CFileOutStream PakOut(OutputPak, IOUtil::BigEndian);
        pak.Repack(PakOut, FileDir, game, mode);
        std::cout << "\n";
    }

    else if ((game == MP3) || (game == DKCR))
    {
        CCorruptionPak pak;
        pak.BuildTOC(f, game);
        std::cout << "Pak will contain " << pak.GetResourceCount() << " files\n";

        CFileOutStream PakOut(OutputPak, IOUtil::BigEndian);
        pak.Repack(PakOut, FileDir, game, mode);
        std::cout << "\n";
    }
}

int main(int argc, char* argv[])
{
    std::cout << "-------------\n"
              << "PakTool v1.02\n"
              << "-------------\n"
              << "\n";

    if (argc < 2)
    {
        Usage();
        return 0;
    }
     /***************
     * Extract Mode *
     ***************/
    else if ((strcmp(argv[1], "-x") == 0) || (strcmp(argv[1], "-xg") == 0))
    {
        int arg;

        // Extract Single
        if (strcmp(argv[1], "-x") == 0)
        {
            if (argc >= 3)
            {
                GlobalPakQueue.emplace_back(std::string (argv[2]));
                arg = 3;
            }
            else
            {
                ExtractionUsage();
                return 1;
            }
        }

        // Extract Game
        else if (strcmp(argv[1], "-xg") == 0)
        {
            if (argc >= 3)
            {
                EWhichGame game = ArgToEnum(argv[2]);
                arg = 3;

                if (!game)
                {
                    std::cout << "Invalid game specified!\n"
                              << "The following game arguments (and some variations) are accepted:\n\n";
                    SupportedPakLists();
                    return 1;
                }

                AddPakList(game);
            }

            else
            {
                std::cout << "Please specify a game.\n\n";
                ExtractionUsage();
                return 1;
            }
        }

        // Optional Arguments
        if (argc >= 3)
        {
            bool handleEmbeddedCompression = false;
            std::string directory;

            while (arg < argc)
            {
                // Handle Embedded Compression
                if (strcmp(argv[arg], "-e") == 0)
                {
                    handleEmbeddedCompression = true;
                    arg++;
                }

                // Set Output Directory
                else if (strcmp(argv[arg], "-o") == 0)
                {
                    if (argc > arg + 1)
                    {
                        directory = argv[arg + 1];
                        arg += 2;
                    }

                    else
                    {
                        std::cout << "Please specify an output directory.\n\n";
                        ExtractionUsage();
                        return 1;
                    }
                }

                else arg++;
            }

            ExtractQueue(directory, handleEmbeddedCompression, Unpack);
            return 0;
        }
    }


    /**************
    * Repack Mode *
    **************/
    else if ((strcmp(argv[1], "-r") == 0) || (strcmp(argv[1], "-ra") == 0))
    {
        EWhichGame game;
        ECompressMode mode;
        std::string directory, pak, list;
        int arg;

        // Repack Normal
        if (strcmp(argv[1], "-r") == 0)
        {
            if (argc >= 6)
            {
                game = ArgToEnum(argv[2]);
                directory = argv[3];
                pak = argv[4];
                list = argv[5];
                mode = Compress;
                arg = 6;
            }

            else
            {
                RepackUsage();
                return 1;
            }
        }

        // Repack Auto
        else if (strcmp(argv[1], "-ra") == 0)
        {
            if (argc >= 4)
            {
                game = ArgToEnum(argv[2]);
                directory = std::string(argv[3]) + "-pak/";
                pak = std::string(argv[3]) + ".pak";
                list = std::string(argv[3]) + "-pak.txt";
                mode = Compress;
                arg = 4;
            }

            else
            {
                RepackUsage();
                return 1;
            }
        }

        // Check Valid Game
        if (!game)
        {
            std::cout << "Invalid game specified!\n"
                      << "The following game arguments (and some variations) are accepted:\n\n";
            SupportedPakLists();
            return 1;
        }

        // Optional Arguments
        while (arg < argc)
        {
            // Handle Embedded Compression
            if (strcmp(argv[arg], "-e") == 0)
            {
                if (mode == Compress)
                    mode = CompressEmbedded;

                else
                {
                    std::cout << "-n and -e cannot be used at the same time.\n\n";
                    RepackUsage();
                    return 1;
                }
            }

            // Disable Compression
            else if (strcmp(argv[arg], "-n") == 0)
            {
                if (mode == Compress)
                    mode = NoCompression;

                else
                {
                    std::cout << "-n and -e cannot be used at the same time.\n\n";
                    RepackUsage();
                    return 1;
                }
            }

            arg++;
        }

        RepackPak(game, mode, directory, pak, list);
        return 0;
    }

    /*****************
    * List Dump Mode *
    *****************/
    else if ((strcmp(argv[1], "-d") == 0) || (strcmp(argv[1], "-dg") == 0))
    {
        if (argc >= 3)
        {
            std::string directory;
            int arg;

            // Dump Single
            if (strcmp(argv[1], "-d") == 0)
            {
                GlobalPakQueue.emplace_back(std::string(argv[2]));
                arg = 3;
            }

            // Dump Game
            else if (strcmp(argv[1], "-dg") == 0)
            {
                if (argc >= 3)
                {
                    EWhichGame game = ArgToEnum(argv[2]);
                    arg = 3;

                    if (!game)
                    {
                        std::cout << "Invalid game specified!\n"
                                  << "The following game arguments (and some variations) are accepted:\n\n";
                        SupportedPakLists();
                        return 1;
                    }

                    AddPakList(game);
                }

                else
                {
                    std::cout << "Please specify a game.\n\n";
                    ListDumpUsage();
                    return 1;
                }
            }


            // Optional Arguments
            while (arg < argc)
            {
                // Set Output Directory
                if (strcmp(argv[arg], "-o") == 0)
                {
                    if (argc > arg + 1)
                    {
                        directory = std::string(argv[arg + 1]);
                        arg += 2;
                    }

                    else
                    {
                        std::cout << "Please specify an output directory.\n\n";
                        ListDumpUsage();
                        return 1;
                    }
                }

                else arg++;
            }

            ExtractQueue(directory, false, ListDump);
        }

        else
        {
            ListDumpUsage();
            return 1;
        }
    }

    // One argument also defaults to unpack; this allows for drag-and-drop unpacking
    else if (argc == 2)
    {
        GlobalPakQueue.emplace_back(std::string(argv[1]));
        ExtractQueue("", false, Unpack);
        return 0;
    }

    else
    {
        std::cout << "Invalid mode specified.\n\n";
        Usage();
        return 1;
    }

    return 0;
}
