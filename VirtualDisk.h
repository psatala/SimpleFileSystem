#ifndef VIRTUALDISK_H_INCLUDED
#define VIRTUALDISK_H_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "Defines.h"




/*********************************************************************
 *                         Virtual Disk class                        *
 *********************************************************************/
/**
        This class handles everything related to the virtual disk.
        Overview of disk architecture:

        average file -> 2 blocks
        each i-node: 128B
            nLinks = 8
            nBlockAddresses = 31
            8B * nLinks = 64B
            2B * nBlockAddresses = 62B (short ints!!!)
            2B other (isDirectory(1b), offset within last block(12b))

        maxFileSize = 124kB

        each i-node block -> 32 files
        32 * nInodeBlocks ~= (freeBlocks - nInodeBlocks) / averageFileSizeInBlocks
        64 nIB = f - nIB
        65 nIB = f
        nIB = f / 65

        maxDiskSize = 128MB <- calculated from size of data bitmap (4kB = 32kb, which gives 32768 data blocks, each 4kB, total 128MB),
        also limited by (unsigned) uint16_t range (65536)
**/


class VirtualDisk
{
    FILE *vDiskFile;
    char* vDiskFileName;
    int vDiskSize;
    int nBlocks;         ///total number of blocks
    int freeBlocks;      ///number of blocks for i-node tables and user data
    int nInodeBlocks;    ///number of blocks for i-node tables
    int iNodeBitmapIndex;
    int dataBitmapIndex;
    int firstINodeIndex;
    int firstDataIndex;

    int currentDirectory;
    int workingDirectory;
    std::vector<std::string> pathToCurrentDir;
    std::vector<std::string> workingPath;


    void openFile();
    void closeFile();
    void prepareBitmaps();

    void setVDiskSize(int newSize);
    int getVDiskSize();
    void setVDiskParameters();

    short int createEmptyDirectory();
    void createRootDirectory();
    void createChildDirectory(uint16_t directoryINumber, char* childName);

    short int findNextFreeBlock();
    short int findNextFreeInode();
    void changeINodeStatus(int iNodeId, bool newStatus);
    void changeBlockStatus(int blockId, bool newStatus);
    bool checkBitFromBitmap(int bitmapId, int entryId);

    short int getINumber(char* fileName, uint16_t directoryINumber);
    void addDirectoryEntry(short int directoryINumber, short int iNumberToAdd, char* fileNameToAdd);
    void deleteDirectoryEntry(short int directoryINumber, char* fileNameToDelete);
    std::vector<std::string> parsePath(std::string path);
    short int specifyWorkingDirectory(std::vector<std::string> parsedPath, int mode = MODE_CD);

    void increaseLinkCount(uint16_t fileINumber);
    void decreaseLinkCount(uint16_t fileINumber);

public:

    VirtualDisk(char* newVDiskFileName = DEFAULT_NAME, int diskSize = -1);
    ~VirtualDisk();

    void copyToVDisk(char* fileNameToCopy, std::string path);
    void copyFromVDisk(std::string path, char* fileNameToCopy);
    void addBytes(std::string path, unsigned int nBytesToAdd);
    void deleteBytes(std::string path, unsigned int nBytesToDelete);
    void deleteFile(std::string path);
    void printDiskUsageInfo();
    void createNewDirectory(std::string path);
    void changeDirectory(std::string path);
    void listDirectory();
    void printPath();
    void addLink(std::string target, std::string linkName);
};






#endif // VIRTUALDISK_H_INCLUDED
