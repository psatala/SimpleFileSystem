#ifndef VIRTUALDISK_H_INCLUDED
#define VIRTUALDISK_H_INCLUDED

#include <iostream>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>


///disk defines
#define BLOCK_SIZE 4096
#define MIN_DISK_SIZE 3 * BLOCK_SIZE
#define MAX_DISK_SIZE 128 * 1024 * 1024
#define AVERAGE_FILE_SIZE_IN_BLOCKS 2
#define MAX_FILE_SIZE_IN_BLOCKS 57
#define N_FILES_PER_I_NODE_BLOCK 32
#define DEFAULT_NAME "vDisk.vdf"

///i-node defines
#define I_NODE_SIZE 128
#define DATA_OFFSET 0
#define NAMES_OFFSET 114
#define OTHER_OFFSET 122
#define BLOCKS_USED_OFFSET 122
#define BYTES_USED_IN_LAST_BLOCK_OFFSET 124
#define IS_CATALOGUE_OFFSET 126
#define NAME_SIZE 8
#define ADDRESS_SIZE 2

///other
#define BYTE_SIZE 8
#define FREE 0
#define USED 1


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
    uint16_t findNextFreeBlock();
    uint16_t findNextFreeInode();
    uint16_t checkByteForFirstZero(uint8_t byte); ///auxiliary function for finding next inode/block
    void changeINodeStatus(int iNodeId, bool newStatus);
    void changeBlockStatus(int blockId, bool newStatus);
    bool checkBitFromBitmap(int bitmapId, int entryId);

public:
    VirtualDisk(char* newVDiskFileName = DEFAULT_NAME);
    ~VirtualDisk();

    void setVDiskSize();
    int getVDiskSize();
    void setVDiskParameters();
    void copyToVDisk(char* fileNameToCopy);
    void copyFromVDisk(char* fileNameToCopy);
};








/********************************************************************************************************************************************************************************************
 *                                                                           private functions                                                                                              *
 ********************************************************************************************************************************************************************************************/


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



uint16_t VirtualDisk::checkByteForFirstZero(uint8_t byte)
{
    for(int i = 0; i < BYTE_SIZE; ++i)
        if(!(byte & (1 << (BYTE_SIZE - 1 - i))))
            return i;
    return -1;
}



uint16_t VirtualDisk::findNextFreeBlock()
{
    bool found = false;
    int indexInByte = -1;
    uint16_t result;
    unsigned char c; ///auxiliary

    fseek(vDiskFile, dataBitmapIndex * BLOCK_SIZE, SEEK_SET);

    for(int i = 0; i < BLOCK_SIZE && !found; ++i)
    {
        c = fgetc(vDiskFile);
        indexInByte = checkByteForFirstZero((uint8_t)c);
        if(indexInByte != -1)
        {
            found = true;
            result = i * BYTE_SIZE + indexInByte;
        }
    }

    return result;
}



uint16_t VirtualDisk::findNextFreeInode()
{
    bool found = false;
    int indexInByte = -1;
    uint16_t result;
    unsigned char c; ///auxiliary

    fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE, SEEK_SET);

    for(int i = 0; i < BLOCK_SIZE && !found; ++i)
    {
        c = fgetc(vDiskFile);
        indexInByte = checkByteForFirstZero((uint8_t)c);
        if(indexInByte != -1)
        {
            found = true;
            result = i * BYTE_SIZE + indexInByte;
        }
    }

    return result;
}



void VirtualDisk::changeBlockStatus(int blockId, bool newStatus)
{
    unsigned char c;
    uint8_t byte;

    ///read
    fseek(vDiskFile, dataBitmapIndex * BLOCK_SIZE + blockId / BYTE_SIZE, SEEK_SET);
    c = fgetc(vDiskFile);

    ///change
    byte = (uint8_t)c;
    if(newStatus)
        byte |= (1 << (blockId % BYTE_SIZE));   ///set
    else
        byte &= ~(1 << (blockId % BYTE_SIZE));  ///unset
    c = (unsigned char)byte;

    ///write
    fseek(vDiskFile, dataBitmapIndex * BLOCK_SIZE + blockId / BYTE_SIZE, SEEK_SET);
    fputc(c, vDiskFile);
}



void VirtualDisk::changeINodeStatus(int iNodeId, bool newStatus)
{
    unsigned char c;
    uint8_t byte;

    ///read
    fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE + iNodeId / BYTE_SIZE, SEEK_SET);
    c = fgetc(vDiskFile);

    ///change
    byte = (uint8_t)c;
    if(newStatus)
        byte |= (1 << (iNodeId % BYTE_SIZE));   ///set
    else
        byte &= ~(1 << (iNodeId % BYTE_SIZE));  ///unset
    c = (unsigned char)byte;

    ///write
    fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE + iNodeId / BYTE_SIZE, SEEK_SET);
    fputc(c, vDiskFile);
}



bool VirtualDisk::checkBitFromBitmap(int bitmapId, int entryId)
{
    unsigned char c;
    uint8_t byte;

    ///read
    fseek(vDiskFile, bitmapId * BLOCK_SIZE + entryId / BYTE_SIZE, SEEK_SET);
    c = fgetc(vDiskFile);

    ///check
    byte = (uint8_t)c;
    return byte & (1 << (entryId % BYTE_SIZE));

}




/********************************************************************************************************************************************************************************************
 *                                                                           public functions                                                                                               *
 ********************************************************************************************************************************************************************************************/


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
    uint16_t iNumber;
    uint16_t blockAddress;
    uint16_t countBlocks = 0;
    int bytesRead;

    ///find next free i-node or terminate when there is none
    iNumber = findNextFreeInode();
    if(-1 == iNumber || iNumber > (BLOCK_SIZE / I_NODE_SIZE) * nInodeBlocks)
    {
        std::cerr << "No free i-node found (too many files)!\n";
        return;
    }
    changeINodeStatus(iNumber, USED);    ///mark i-node as used


    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + NAMES_OFFSET, SEEK_SET); ///set file pointer to start of names
    fwrite(fileNameToCopy, 1, NAME_SIZE, vDiskFile);                                                 ///add name

    fileToCopy = fopen(fileNameToCopy, "rb+");

    while((bytesRead = fread(buffer, 1, BLOCK_SIZE, fileToCopy)) > 0 && countBlocks < MAX_FILE_SIZE_IN_BLOCKS)
    {
        if(ferror(fileToCopy))
        {
            std::cerr << "Error reading file to copy!\n";
            fclose(fileToCopy);
            return;
        }

        ///note how many bytes in last block used
        if(bytesRead != BLOCK_SIZE)  ///no ferror here
        {
            fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + BYTES_USED_IN_LAST_BLOCK_OFFSET, SEEK_SET);
            fwrite((const void*) &bytesRead, sizeof(bytesRead), 1, vDiskFile);
        }

        ++countBlocks;

        blockAddress = findNextFreeBlock(); ///find next free block
        if(-1 == blockAddress || blockAddress > freeBlocks - nInodeBlocks)
        {
            std::cerr << "No free block found (not enough free space)! Copying file stopped.\n";
            fclose(fileToCopy);
            return;
        }
        changeBlockStatus(blockAddress, USED);  ///mark data block as used

        ///add block address to i-node table
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + countBlocks * sizeof(blockAddress), SEEK_SET);  ///set file pointer to next free block address
        fwrite((const void* ) &blockAddress, sizeof(blockAddress), 1, vDiskFile);

        ///write data from buffer
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE, SEEK_SET);
        fwrite(buffer, 1, bytesRead, vDiskFile);
    }

    ///note how many blocks were used
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + BLOCKS_USED_OFFSET, SEEK_SET);
    fwrite((const void*) &countBlocks, sizeof(countBlocks), 1, vDiskFile);

    fclose(fileToCopy);
}



void VirtualDisk::copyFromVDisk(char* fileNameToCopy)
{
    FILE* fileToCopy;
    unsigned char* buffer = new unsigned char [BLOCK_SIZE]; ///auxiliary buffer to store data
    uint16_t blockAddress;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    uint16_t iNumber;
    uint16_t expectedBytes;
    bool found = false;

    std::string nameToFind = (std::string)fileNameToCopy;
    std::string temporaryString;
    char* nameBuffer = new char [NAME_SIZE];
    nameToFind = nameToFind.substr(0, NAME_SIZE);

    for(int i = 0; i < BLOCK_SIZE && !found; ++i)
    {
        if(checkBitFromBitmap(iNodeBitmapIndex, i)) ///i-node in use
        {
            fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + i * I_NODE_SIZE + NAMES_OFFSET, SEEK_SET);
            fread(nameBuffer, 1, NAME_SIZE, vDiskFile);
            temporaryString = (std::string)nameBuffer;
            if(0 == nameToFind.compare(temporaryString)) ///name found
            {
                found = true;
                iNumber = i;
            }

        }
    }

    if(!found)
    {
        std::cerr << "No such file exists!\n";
        return;
    }


    ///read how many blocks were used
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + BLOCKS_USED_OFFSET, SEEK_SET);
    fread(&countBlocks, sizeof(countBlocks), 1, vDiskFile);

    ///read how many bites in last block were used
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + BYTES_USED_IN_LAST_BLOCK_OFFSET, SEEK_SET);
    fread(&bytesUsedInLastBlock, sizeof(bytesUsedInLastBlock), 1, vDiskFile);


    fileToCopy = fopen("copiedFile", "wb+");
    for(int i = 0; i < countBlocks; ++i)
    {
        if(i < countBlocks - 1) ///normal case
            expectedBytes = BLOCK_SIZE;
        else                    ///last block case
            expectedBytes = bytesUsedInLastBlock;


        ///read block address
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + i * ADDRESS_SIZE, SEEK_SET);
        fread(&blockAddress, sizeof(uint16_t), 1, vDiskFile);

        ///read block content into buffer
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE, SEEK_SET);


        if(expectedBytes != fread(buffer, 1, expectedBytes, vDiskFile))
        {
            std::cerr << "Could not read the entire block!\n";
            fclose(fileToCopy);
            return;
        }
        ///write buffer into file
        fseek(fileToCopy, 0, SEEK_END);
        fwrite(buffer, 1, expectedBytes, fileToCopy);
    }

    fclose(fileToCopy);
}


#endif // VIRTUALDISK_H_INCLUDED
