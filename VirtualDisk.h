#ifndef VIRTUALDISK_H_INCLUDED
#define VIRTUALDISK_H_INCLUDED

#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

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


    void openFile();
    void closeFile();
    void prepareBitmaps();
    void createRootDirectory();

    uint16_t findNextFreeBlock();
    uint16_t findNextFreeInode();
    void changeINodeStatus(int iNodeId, bool newStatus);
    void changeBlockStatus(int blockId, bool newStatus);
    bool checkBitFromBitmap(int bitmapId, int entryId);

    uint16_t getINumber(char* fileName);
    void addDirectoryEntry(uint16_t directoryINumber, uint16_t iNumberToAdd, char* fileNameToAdd);

public:
    VirtualDisk(char* newVDiskFileName = DEFAULT_NAME);
    ~VirtualDisk();

    void setVDiskSize();
    int getVDiskSize();
    void setVDiskParameters();
    void copyToVDisk(char* fileNameToCopy);
    void copyFromVDisk(char* fileNameToCopy);
    void addBytes(char* fileName, unsigned int nBytesToAdd);
    void deleteBytes(char* fileName, unsigned int nBytesToDelete);
    void deleteFile(char* fileNameToDelete);
    void printDiskUsageInfo();
    void listDirectory(uint16_t directoryINumber);

};






#endif // VIRTUALDISK_H_INCLUDED
