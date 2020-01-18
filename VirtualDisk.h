#ifndef VIRTUALDISK_H_INCLUDED
#define VIRTUALDISK_H_INCLUDED

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#define BLOCK_SIZE 4096
#define MIN_DISK_SIZE 3 * BLOCK_SIZE
#define MAX_DISK_SIZE 128 * 1024 * 1024
#define AVERAGE_FILE_SIZE_IN_BLOCKS 2
#define N_FILES_PER_I_NODE_BLOCK 32

#define DEFAULT_NAME "vDisk.vdf"


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
        also limited by (unsigned) short int range (65536)
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
    short int findNextFreeBlock();
    short int findNextFreeInode();
    short int checkBiteForFirstZero(uint8_t byte); ///auxiliary function for finding next inode/block
public:
    VirtualDisk(char* newVDiskFileName = DEFAULT_NAME);
    ~VirtualDisk();

    void setVDiskSize();
    int getVDiskSize();
    void setVDiskParameters();
    void copyToVDisk(char* fileNameToCopy);
};




VirtualDisk::VirtualDisk(char* newVDiskFileName)
{
    vDiskFileName = newVDiskFileName;
    openFile();
    setVDiskSize();
    setVDiskParameters();
    prepareBitmaps();
}



VirtualDisk::~VirtualDisk()
{
    closeFile();
}



void VirtualDisk::openFile()
{

    if(access(vDiskFileName, F_OK) != -1)       ///file exists
        vDiskFile = fopen(vDiskFileName, "rb+");
    else                                        ///file does not exist
        vDiskFile = fopen(vDiskFileName, "wb+");

    if(vDiskFile == NULL)
    {
        std::cerr << "Could not open virtual disk file!\n";
        exit(EXIT_FAILURE);
    }

}



void VirtualDisk::closeFile()
{
    fclose(vDiskFile);
}



void VirtualDisk::prepareBitmaps()
{

    fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE, SEEK_SET);
    for(int i = 0; i < 2 * BLOCK_SIZE; ++i)
    {
        fputc('\0', vDiskFile);
    }
}



short int VirtualDisk::checkBiteForFirstZero(uint8_t byte)
{
    const int byteSize = 8;
    for(int i = 0; i < byteSize; ++i)
        if(!(byte & (1 << (byteSize - i))))
            return i;
    return -1;
}



short int VirtualDisk::findNextFreeInode()
{
    bool found = false;
    int res;
    unsigned char c; ///auxiliary
    fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE, SEEK_SET);

    while(!found)
    {
        c = fgetc(vDiskFile);
        res = checkBiteForFirstZero((uint8_t)c);
        if(res != -1)
            found = true;
    }
}



void VirtualDisk::setVDiskSize()
{
    int newSize;

    std::cout << "Please specify size of virtual disk in bites: ";
    std::cin >> newSize;

    newSize = newSize - newSize % BLOCK_SIZE;      ///resize to be a multiply of block size
    newSize = std::min(newSize, MAX_DISK_SIZE);    ///resize if size bigger than allowed
    newSize = std::max(newSize, MIN_DISK_SIZE);    ///resize if size smaller than allowed

    vDiskSize = newSize;

    fseek(vDiskFile, newSize - 1, SEEK_SET);
    fprintf(vDiskFile, "x");
}



int VirtualDisk::getVDiskSize()
{
    return vDiskSize;
}



///based on disk size, calculate where bitmaps, i-node table and user data are
void VirtualDisk::setVDiskParameters()
{
    vDiskSize = getVDiskSize();
    nBlocks = vDiskSize / BLOCK_SIZE;
    freeBlocks = nBlocks - 2;
    nInodeBlocks = freeBlocks / (N_FILES_PER_I_NODE_BLOCK * AVERAGE_FILE_SIZE_IN_BLOCKS + 1);
    nInodeBlocks = std::max(nInodeBlocks, 1);   ///at least one i-node block must be present
    iNodeBitmapIndex = 0;
    dataBitmapIndex = 1;
    firstINodeIndex = 2;
    firstDataIndex = nInodeBlocks + firstINodeIndex;
}



void VirtualDisk::copyToVDisk(char* fileNameToCopy)
{
    FILE* fileToCopy;
    unsigned char* buffer = new unsigned char [BLOCK_SIZE]; ///auxiliary buffer to store data

    ///find next free i-node or terminate when there is none

    fileToCopy = fopen(fileNameToCopy, "rb+");

    while(BLOCK_SIZE == fread(buffer, 1, BLOCK_SIZE, fileToCopy))
    {
        ///find next free block
        ///add block address to i-node table
        ///write data from buffer
    }

}



#endif // VIRTUALDISK_H_INCLUDED
