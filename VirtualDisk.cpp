#include "VirtualDisk.h"



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



void VirtualDisk::createRootDirectory()
{
    uint16_t iNumber = findNextFreeInode();
    uint16_t blockAddress = findNextFreeBlock();
    bool isDirectory = true;
    uint16_t directorySize = 0;
    uint16_t linkCount = 0;


    if(-1 == iNumber || -1 == blockAddress)
    {
        std::cerr << "Could not create root directory!\n";
        return;
    }

    ///add block address to i-node table
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fwrite((const void* ) &blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///note that this is a directory
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + IS_DIRECTORY_OFFSET, SEEK_SET);
    fwrite((const void* ) &isDirectory, sizeof(isDirectory), 1, vDiskFile);

    ///write directory size (empty)
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void* ) &directorySize, sizeof(directorySize), 1, vDiskFile);

    ///write directory link count
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fwrite((const void* ) &linkCount, sizeof(linkCount), 1, vDiskFile);


    ///add .
    addDirectoryEntry(iNumber, iNumber, ".");

    ///add ..
    addDirectoryEntry(iNumber, iNumber, "..");

}



void VirtualDisk::addDirectoryEntry(uint16_t directoryINumber, uint16_t iNumberToAdd, char* fileNameToAdd)
{
    uint16_t blockAddress;
    uint16_t sizeOfDirectory;
    uint16_t linkCount;

    ///read directory block address
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fread(&blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///read size
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

    ///write i-number
    fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + sizeOfDirectory, SEEK_SET);
    fwrite((const void* ) &iNumberToAdd, sizeof(iNumberToAdd), 1, vDiskFile);

    ///write name
    fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + sizeOfDirectory + DIRECTORY_NAME_OFFSET, SEEK_SET);
    fwrite(fileNameToAdd, 1, DIRECTORY_NAME_SIZE, vDiskFile);

    ///add link
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumberToAdd * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fread(&linkCount, sizeof(linkCount), 1, vDiskFile);
    ++linkCount;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumberToAdd * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fwrite((const void* ) &linkCount, sizeof(linkCount), 1, vDiskFile);

    ///update directory size
    sizeOfDirectory += DIRECTORY_ENTRY_SIZE;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void* ) &sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

}



uint16_t VirtualDisk::findNextFreeBlock()
{
    bool found = false;
    int result = -1;

    for(int i = 0; i < nBlocks - firstDataIndex && !found; ++i)
    {
        if(!checkBitFromBitmap(dataBitmapIndex, i))
        {
            found = true;
            result = i;
        }
    }

    return result;
}



uint16_t VirtualDisk::findNextFreeInode()
{
    bool found = false;
    int result = -1;

    for(int i = 0; i < nInodeBlocks * BLOCK_SIZE / I_NODE_SIZE && !found; ++i)
    {
        if(!checkBitFromBitmap(iNodeBitmapIndex, i))
        {
            found = true;
            result = i;
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



uint16_t VirtualDisk::getINumber(char* fileName)
{
    bool found = false;
    int iNumber = -1;
    std::string nameToFind = (std::string)fileName;
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

    return iNumber;
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
    createRootDirectory();
}



VirtualDisk::~VirtualDisk()
{
    closeFile();
}



void VirtualDisk::setVDiskSize()
{
    int newSize;

    std::cout << "Please specify size of virtual disk in bytes: ";
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
    uint16_t bytesUsedInLastBlock;
    uint32_t fileSize;
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
        bytesUsedInLastBlock = bytesRead;

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
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + (countBlocks - 1) * sizeof(blockAddress), SEEK_SET);  ///set file pointer to next free block address
        fwrite((const void* ) &blockAddress, sizeof(blockAddress), 1, vDiskFile);

        ///write data from buffer
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE, SEEK_SET);
        fwrite(buffer, 1, bytesRead, vDiskFile);
    }

    ///note file size
    fileSize = (uint32_t)(countBlocks - 1) * BLOCK_SIZE + (uint32_t)bytesUsedInLastBlock;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void*) &fileSize, sizeof(fileSize), 1, vDiskFile);

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
    uint32_t fileSize;

    iNumber = getINumber(fileNameToCopy);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }


    ///read size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&fileSize, sizeof(fileSize), 1, vDiskFile);


    ///calculate count blocks and how many bytes in last block were used
    bytesUsedInLastBlock = (uint16_t)(fileSize % BLOCK_SIZE);
    countBlocks = (uint16_t)(fileSize / BLOCK_SIZE);
    if(bytesUsedInLastBlock) ///last block not empty
        ++countBlocks;


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



void VirtualDisk::deleteFile(char* fileNameToDelete)
{
    uint16_t blockAddress;
    uint16_t countBlocks;
    uint32_t fileSize;
    uint16_t iNumber;

    iNumber = getINumber(fileNameToDelete);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }


    ///read size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&fileSize, sizeof(fileSize), 1, vDiskFile);

    ///calculate count blocks
    countBlocks = (uint16_t)(fileSize / BLOCK_SIZE);
    if(fileSize % BLOCK_SIZE) ///last block not empty
        ++countBlocks;



    for(int i = 0; i < countBlocks; ++i)
    {
        ///read block address
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + i * ADDRESS_SIZE, SEEK_SET);
        fread(&blockAddress, sizeof(uint16_t), 1, vDiskFile);

        ///free block
        changeBlockStatus(blockAddress, FREE);
    }

    ///free i-node
    changeINodeStatus(iNumber, FREE);
}



void VirtualDisk::addBytes(char* fileName, unsigned int nBytesToAdd)
{
    uint16_t iNumber;
    uint32_t oldFileSize;
    uint32_t newFileSize;
    uint16_t nBlocksToAllocate = 0;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    uint16_t blockAddress;

    iNumber = getINumber(fileName);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }


    ///read size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&oldFileSize, sizeof(oldFileSize), 1, vDiskFile);

    newFileSize = oldFileSize + nBytesToAdd;

    ///write size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void*) &newFileSize, sizeof(newFileSize), 1, vDiskFile);

    bytesUsedInLastBlock = oldFileSize % BLOCK_SIZE;
    countBlocks = oldFileSize / BLOCK_SIZE;
    if(bytesUsedInLastBlock)
        ++countBlocks;

    ///calculate how many last blocks should be allocated
    if(bytesUsedInLastBlock)  ///if last block was not empty
        nBytesToAdd -= (BLOCK_SIZE - bytesUsedInLastBlock);

    nBlocksToAllocate += (nBytesToAdd / BLOCK_SIZE); ///other full blocks
    if(nBytesToAdd % BLOCK_SIZE)
        ++nBlocksToAllocate;

    for(int i = 1; i <= nBlocksToAllocate; ++i)
    {
        blockAddress = findNextFreeBlock(); ///find next free block
        if(-1 == blockAddress || blockAddress > freeBlocks - nInodeBlocks)
        {
            std::cerr << "No free block found (not enough free space)! Adding bytes stopped.\n";
            return;
        }
        changeBlockStatus(blockAddress, USED);  ///mark data block as used

        ///add block address to i-node table
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + (countBlocks + i) * sizeof(blockAddress), SEEK_SET);  ///set file pointer to next free block address
        fwrite((const void* ) &blockAddress, sizeof(blockAddress), 1, vDiskFile);

    }

}



void VirtualDisk::deleteBytes(char* fileName, unsigned int nBytesToDelete)
{
    uint16_t iNumber;
    uint32_t oldFileSize;
    uint32_t newFileSize;
    uint16_t nBlocksToFree = 0;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    uint16_t blockAddress;

    iNumber = getINumber(fileName);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }


    ///read size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&oldFileSize, sizeof(oldFileSize), 1, vDiskFile);

    newFileSize = oldFileSize - nBytesToDelete;

    ///write size of file
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void*) &newFileSize, sizeof(newFileSize), 1, vDiskFile);

    bytesUsedInLastBlock = oldFileSize % BLOCK_SIZE;
    countBlocks = oldFileSize / BLOCK_SIZE;
    if(bytesUsedInLastBlock)
        ++countBlocks;

    ///calculate how many last blocks should be freed
    if(nBytesToDelete >= bytesUsedInLastBlock) ///last block should be freed
    {
        ++nBlocksToFree;
        nBytesToDelete -= bytesUsedInLastBlock;
    }
    nBlocksToFree += (nBytesToDelete / BLOCK_SIZE); ///other full blocks
    nBlocksToFree = std::min(nBlocksToFree, countBlocks); ///no more than allocated blocks should be freed

    for(int i = 0; i < nBlocksToFree; ++i)
    {
        ///read block address
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + DATA_OFFSET + (countBlocks - i - 1) * ADDRESS_SIZE, SEEK_SET);
        fread(&blockAddress, sizeof(uint16_t), 1, vDiskFile);

        ///free block
        changeBlockStatus(blockAddress, FREE);
    }

}



void VirtualDisk::printDiskUsageInfo()
{
    int nInodesTotal = nInodeBlocks * BLOCK_SIZE / I_NODE_SIZE;
    int nDataBlocksTotal = nBlocks - firstDataIndex;
    int nInodesInUse = 0;
    int nDataBlocksInUse = 0;
    int sizeForUserDataTotal = nDataBlocksTotal * BLOCK_SIZE;
    int sizeForUserDataInUse = 0;
    int fileSize; ///auxiliary

    ///count i-nodes and size of user data in use
    for(int i = 0; i < nInodesTotal; ++i)
    {
        if(checkBitFromBitmap(iNodeBitmapIndex, i))
        {
            ++nInodesInUse;

            ///read size of file
            fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + i * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
            fread(&fileSize, sizeof(fileSize), 1, vDiskFile);
            sizeForUserDataInUse += fileSize;
        }
    }

    ///count data blocks in use
    for(int i = 0; i < nDataBlocksTotal; ++i)
        if(checkBitFromBitmap(dataBitmapIndex, i))
            ++nDataBlocksInUse;

    std::cout << "Usage of space (in bytes): " << sizeForUserDataInUse << "/" << sizeForUserDataTotal << "\n";
    std::cout << "Usage of data blocks: " << nDataBlocksInUse << "/" << nDataBlocksTotal << "\n";
    std::cout << "Usage of i-nodes: " << nInodesInUse << "/" << nInodesTotal << "\n\n";
}



void VirtualDisk::listDirectory(uint16_t directoryINumber)
{
    uint16_t blockAddress;
    uint16_t sizeOfDirectory;
    uint16_t entryINumber;
    uint16_t entryLinkCount;
    uint16_t entrySize;
    bool entryFileType;
    char* entryName;

    ///read directory block address
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fread(&blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///read size
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

    for(int i = 0; i < sizeOfDirectory / DIRECTORY_ENTRY_SIZE; ++i)
    {
        ///print entry i-number
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + DIRECTORY_ENTRY_SIZE * i + DIRECTORY_I_NUMBER_OFFSET, SEEK_SET);
        fread(&entryINumber, sizeof(entryINumber), 1, vDiskFile);
        std::cout << entryINumber << " ";

        ///print entry link count
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + entryINumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
        fread(&entryLinkCount, sizeof(entryLinkCount), 1, vDiskFile);
        std::cout << entryLinkCount << " ";

        ///print entry size
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + entryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
        fread(&entrySize, sizeof(entrySize), 1, vDiskFile);
        std::cout << entrySize << " ";

        ///print file type
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + entryINumber * I_NODE_SIZE + IS_DIRECTORY_OFFSET, SEEK_SET);
        fread(&entryFileType, sizeof(entryFileType), 1, vDiskFile);
        if(entryFileType)
            std::cout << "directory ";
        else
            std::cout << "file ";

        ///print entry name
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + DIRECTORY_ENTRY_SIZE * i + DIRECTORY_NAME_OFFSET, SEEK_SET);
        fread(entryName, sizeof(char), DIRECTORY_NAME_SIZE, vDiskFile);
        puts(entryName);
    }
}
