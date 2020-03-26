///Name: VirtualDisk.cpp
///Purpose: define methods from VirtualDisk class - the most important class of the program



#include "VirtualDisk.h"



/********************************************************************************************************************************************************************************************
 *                                                                           private methods                                                                                                *
 ********************************************************************************************************************************************************************************************/



///function opens file from user system be used for virtual disk implementation
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



///function closes file from user system be used for virtual disk implementation
void VirtualDisk::closeFile()
{
    fclose(vDiskFile);
}



///function clears i-node and data bitmaps during virtual disk creation
void VirtualDisk::prepareBitmaps()
{
    if(0 == findNextFreeInode())   ///root directory not yet created - file sytem being created, not restored
    {
        fseek(vDiskFile, iNodeBitmapIndex * BLOCK_SIZE, SEEK_SET);
        for(int i = 0; i < 2 * BLOCK_SIZE; ++i)
        {
            fputc('\0', vDiskFile);
        }
    }
}



///function sets virtual disk size
///parameters: new size of virtual disk (in bytes)
void VirtualDisk::setVDiskSize(int newSize)
{

    if(-1 == newSize)
    {
        std::cout << "Please specify size of virtual disk in bytes: ";
        std::cin >> newSize;
    }

    newSize = newSize - newSize % BLOCK_SIZE;      ///resize to be a multiply of block size
    newSize = std::min(newSize, MAX_DISK_SIZE);    ///resize if size bigger than allowed
    newSize = std::max(newSize, MIN_DISK_SIZE);    ///resize if size smaller than allowed

    vDiskSize = newSize;

    fseek(vDiskFile, newSize - 1, SEEK_SET);
    fprintf(vDiskFile, "x");
}



///function gets virtual disk size
///return value: size of virtual disk
int VirtualDisk::getVDiskSize()
{
    return vDiskSize;
}



///function sets virtual disk parameters during virtual disk creation - based on disk size, calculate where bitmaps, i-node table and user data are
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



///function creates empty directory
///return value: i-number of created directory
short int VirtualDisk::createEmptyDirectory()
{
    short int iNumber = findNextFreeInode();
    uint16_t blockAddress = findNextFreeBlock();
    bool isDirectory = true;
    uint16_t directorySize = 0;
    uint16_t linkCount = 0;


    if(-1 == iNumber || -1 == blockAddress)
    {
        std::cerr << "Could not create root directory!\n";
        exit(EXIT_FAILURE);
    }
    changeINodeStatus(iNumber, USED);
    changeBlockStatus(blockAddress, USED);

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

    return iNumber;
}



///function creates root directory
void VirtualDisk::createRootDirectory()
{
    if(0 == findNextFreeInode()) ///root directory not yet created - file sytem being created, not restored
    {
        uint16_t rootINumber = createEmptyDirectory();
        currentDirectory = rootINumber;

        addDirectoryEntry(rootINumber, rootINumber, ".");
        addDirectoryEntry(rootINumber, rootINumber, "..");
    }
}



///function creates child directory
///parameters: i-number of parent directory, name of child directory
void VirtualDisk::createChildDirectory(uint16_t directoryINumber, char* childName)
{
    uint16_t childDirectoryINumber = createEmptyDirectory();
    addDirectoryEntry(childDirectoryINumber, childDirectoryINumber, ".");
    addDirectoryEntry(childDirectoryINumber, directoryINumber, "..");
    addDirectoryEntry(directoryINumber, childDirectoryINumber, childName);
}



///function adds directory entry
///parameters: i-number of directory to add in. i-number of file to add, name of file to add
void VirtualDisk::addDirectoryEntry(short int directoryINumber, short int iNumberToAdd, char* fileNameToAdd)
{
    uint16_t blockAddress;
    uint16_t sizeOfDirectory;
    uint16_t linkCount;

    ///read directory block address
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fread(&blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///read directory size
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

    if(sizeOfDirectory / DIRECTORY_ENTRY_SIZE >= DIRECTORY_MAX_ENTRIES)
    {
        std::cerr << "Directory already full!\n";
        return;
    }

    ///write i-number
    fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + sizeOfDirectory, SEEK_SET);
    fwrite((const void* ) &iNumberToAdd, sizeof(iNumberToAdd), 1, vDiskFile);

    ///write name
    fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + sizeOfDirectory + DIRECTORY_NAME_OFFSET, SEEK_SET);
    fwrite(fileNameToAdd, 1, DIRECTORY_NAME_SIZE, vDiskFile);

    ///add link
    increaseLinkCount(iNumberToAdd);

    ///update directory size
    sizeOfDirectory += DIRECTORY_ENTRY_SIZE;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void* ) &sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

}



///function deletes directory entry
///parameters: i-number of directory to delete from, name of file to delete
void VirtualDisk::deleteDirectoryEntry(short int directoryINumber, char* fileNameToDelete)
{
    uint16_t blockAddress;
    uint16_t sizeOfDirectory;
    uint16_t linkCount;
    uint16_t index;
    char* buffer = new char [DIRECTORY_NAME_SIZE];
    short int iNumberToMove;

    ///read directory block address
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fread(&blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///read directory size
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

    ///specify file position within directory
    for(index = 0; index < sizeOfDirectory / DIRECTORY_ENTRY_SIZE; ++index)
    {
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + index * DIRECTORY_ENTRY_SIZE + DIRECTORY_NAME_OFFSET, SEEK_SET);
        fread(buffer, 1, DIRECTORY_NAME_SIZE, vDiskFile);
        if(0 == strcmp(buffer, fileNameToDelete))
            break;
    }

    ///move all next entries back one position
    while(index < sizeOfDirectory / DIRECTORY_ENTRY_SIZE - 1)
    {
        ///moving i-number
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + (index + 1) * DIRECTORY_ENTRY_SIZE + DIRECTORY_I_NUMBER_OFFSET, SEEK_SET);
        fread(&iNumberToMove, sizeof(iNumberToMove), 1, vDiskFile);
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + index * DIRECTORY_ENTRY_SIZE + DIRECTORY_I_NUMBER_OFFSET, SEEK_SET);
        fwrite((const void* ) &iNumberToMove, sizeof(iNumberToMove), 1, vDiskFile);

        ///moving name
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + (index + 1) * DIRECTORY_ENTRY_SIZE + DIRECTORY_NAME_OFFSET, SEEK_SET);
        fread(buffer, 1, DIRECTORY_NAME_SIZE, vDiskFile);
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + index * DIRECTORY_ENTRY_SIZE + DIRECTORY_NAME_OFFSET, SEEK_SET);
        fwrite(buffer, 1, DIRECTORY_NAME_SIZE, vDiskFile);

        ++index;
    }

    ///update directory size
    sizeOfDirectory -= DIRECTORY_ENTRY_SIZE;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fwrite((const void* ) &sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);
}



///function finds next free data block
///return value: index of first free data block
short int VirtualDisk::findNextFreeBlock()
{
    bool found = false;
    short int result = -1;

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



///function finds next free i-node
///return value: first free i-number
short int VirtualDisk::findNextFreeInode()
{
    bool found = false;
    short int result = -1;

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



///function changes data block status
///parameters: index of data block to change, new status (free or used)
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



///function changes i-node status
///parameters: i-number to change, new status (free or used)
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



///function checks status of a bit from a given bitmap
///parameters: id of bitmap (i-node or data block), id of entry on that bitmap
///return value: status of bit (free or used)
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



///function gets i-number of given file from a given directory
///parameters: name of file, i-number of directory to search in
///return value: i-number of given file
short int VirtualDisk::getINumber(char* fileName, uint16_t directoryINumber)
{
    bool found = false;
    short int iNumber = -1;
    std::string nameToFind = (std::string)fileName;
    std::string temporaryString;
    char* nameBuffer = new char [NAME_SIZE];
    nameToFind = nameToFind.substr(0, NAME_SIZE);

    uint16_t blockAddress;
    uint16_t sizeOfDirectory;


    ///read directory block address
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + DATA_OFFSET, SEEK_SET);
    fread(&blockAddress, sizeof(blockAddress), 1, vDiskFile);

    ///read directory size
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + directoryINumber * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
    fread(&sizeOfDirectory, sizeof(sizeOfDirectory), 1, vDiskFile);

    for(int i = 0; i < sizeOfDirectory / DIRECTORY_ENTRY_SIZE && !found; ++i)
    {
        ///read name
        fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + i * DIRECTORY_ENTRY_SIZE + DIRECTORY_NAME_OFFSET, SEEK_SET);
        fread(nameBuffer, 1, DIRECTORY_NAME_SIZE, vDiskFile);

        temporaryString = (std::string)nameBuffer;
        if(0 == nameToFind.compare(temporaryString)) ///name found
        {
            found = true;
            fseek(vDiskFile, (firstDataIndex + blockAddress) * BLOCK_SIZE + i * DIRECTORY_ENTRY_SIZE + DIRECTORY_I_NUMBER_OFFSET, SEEK_SET);
            fread(&iNumber, sizeof(iNumber), 1, vDiskFile);
        }
    }

    return iNumber;
}



///function parses path - from string to vector
///parameters: path in string form
///return value: path in vector form
std::vector<std::string> VirtualDisk::parsePath(std::string path)
{
    std::vector<std::string> parsedPath;
    std::string temporary;
    for(int i = 0; i < path.size(); ++i)
    {
        if(path[i] == '/' && !temporary.empty())
        {
            parsedPath.push_back(temporary);
            temporary.clear();
        }
        else if(path[i] != '/')
            temporary.push_back(path[i]);
    }
    if(!temporary.empty())
        parsedPath.push_back(temporary);

    return parsedPath;
}



///function interprets parsed path to specify working (temporary current) directory
///parameters: parsed path in vector form, mode of specifying (whether to interpret last element of parsed path or not)
///return value: -1 if could not resolve parsed path
short int VirtualDisk::specifyWorkingDirectory(std::vector<std::string> parsedPath, int mode)
{
    bool isDirectory;
    int limit;  ///how far to go

    workingDirectory = currentDirectory; ///start from current directory
    workingPath = pathToCurrentDir;      ///start with current path

    if(MODE_CD == mode)
        limit = parsedPath.size();       ///go through everything - just like in cd command
    else
        limit = parsedPath.size() - 1;   ///go through everything but last one - just like in mkdir command

    for(int i = 0; i < limit; ++i)
    {
        ///find i-number for this directory
        workingDirectory = getINumber((char*)parsedPath[i].c_str(), (uint16_t)workingDirectory);

        ///update working path
        if(parsedPath[i] == ".." && !workingPath.empty())
            workingPath.pop_back();
        else if(parsedPath[i] != "." && parsedPath[i] != "..")
            workingPath.push_back(parsedPath[i]);

        ///check if it is a directory
        fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + workingDirectory * I_NODE_SIZE + IS_DIRECTORY_OFFSET, SEEK_SET);
        fread(&isDirectory, sizeof(isDirectory), 1, vDiskFile);

        ///exit if could not find
        if(-1 == workingDirectory || !isDirectory)
        {
            std::cerr << parsedPath[i] << ": no such directory!\n";
            return -1;
        }
    }
}



///function increases link count of a given file
///parameters: i-number of file to increase link counter
void VirtualDisk::increaseLinkCount(uint16_t fileINumber)
{
    uint16_t linkCount;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + fileINumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fread(&linkCount, sizeof(linkCount), 1, vDiskFile);
    ++linkCount;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + fileINumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fwrite((const void* ) &linkCount, sizeof(linkCount), 1, vDiskFile);
}



///function decreases link count of a given file
///parameters: i-number of file to decrease link counter
void VirtualDisk::decreaseLinkCount(uint16_t fileINumber)
{
    uint16_t linkCount;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + fileINumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fread(&linkCount, sizeof(linkCount), 1, vDiskFile);
    --linkCount;
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + fileINumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fwrite((const void* ) &linkCount, sizeof(linkCount), 1, vDiskFile);
}





/********************************************************************************************************************************************************************************************
 *                                                                           public methods                                                                                                 *
 ********************************************************************************************************************************************************************************************/



///constructor
///parameters: name of virtual disk file, size of virtual disk file
VirtualDisk::VirtualDisk(char* newVDiskFileName, int diskSize)
{
    vDiskFileName = newVDiskFileName;
    openFile();
    setVDiskSize(diskSize);
    setVDiskParameters();
    prepareBitmaps();
    createRootDirectory();
}



///destructor
VirtualDisk::~VirtualDisk()
{
    closeFile();
}



///function copies file from user system to virtual disk
///parameters: name of file to copy from user system, path to target location
void VirtualDisk::copyToVDisk(char* fileNameToCopy, std::string path)
{
    FILE* fileToCopy;
    unsigned char* buffer = new unsigned char [BLOCK_SIZE]; ///auxiliary buffer to store data
    short int iNumber;
    uint16_t blockAddress;
    uint16_t countBlocks = 0;
    uint16_t linkCount = 0;
    uint16_t checkLinkCount;
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

    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fwrite((const void* ) &linkCount, sizeof(linkCount), 1, vDiskFile);  ///set link count to 0

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);
    addDirectoryEntry(workingDirectory, iNumber, (char*)parsedPath.back().c_str());        ///this will increment link count

    fileToCopy = fopen(fileNameToCopy, "rb+");
    if(NULL == fileToCopy)
    {
        std::cerr << "Could not open file!\n";
        return;
    }

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



///function copies file from virtual disk to user system
///parameters: path to file on virtual disk, name of target file on user system
void VirtualDisk::copyFromVDisk(std::string path, char* fileNameToCopy)
{
    FILE* fileToCopy;
    unsigned char* buffer = new unsigned char [BLOCK_SIZE]; ///auxiliary buffer to store data
    uint16_t blockAddress;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    short int iNumber;
    uint16_t expectedBytes;
    uint32_t fileSize;

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);

    iNumber = getINumber((char*)parsedPath.back().c_str(), workingDirectory);
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


    fileToCopy = fopen(fileNameToCopy, "wb+");
    if(NULL == fileToCopy)
    {
        std::cerr << "Could not open file!\n";
        return;
    }

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



///function deletes a file from virtual disk
///parameters: path to file to delete
void VirtualDisk::deleteFile(std::string path)
{
    uint16_t blockAddress;
    uint16_t countBlocks;
    uint32_t fileSize;
    short int iNumber;
    uint16_t linkCount;
    bool isDirectory;

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);

    iNumber = getINumber((char*)parsedPath.back().c_str(), workingDirectory);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }

    ///check if it is a directory
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + IS_DIRECTORY_OFFSET, SEEK_SET);
    fread(&isDirectory, sizeof(isDirectory), 1, vDiskFile);
    if(isDirectory)
    {
        std::cerr << "Given file is a directory (removing directories not yet supported)!\n";
        return;
    }

    deleteDirectoryEntry(workingDirectory, (char*)parsedPath.back().c_str());

    decreaseLinkCount(iNumber);
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + LINK_COUNT_OFFSET, SEEK_SET);
    fread(&linkCount, sizeof(linkCount), 1, vDiskFile);
    if(linkCount > 0) ///other links point to this file, cannot delete
        return;

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



///function adds null bytes to the end of given file
///parameters: path to file, number of bytes to add
void VirtualDisk::addBytes(std::string path, unsigned int nBytesToAdd)
{
    short int iNumber;
    uint32_t oldFileSize;
    uint32_t newFileSize;
    uint16_t nBlocksToAllocate = 0;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    uint16_t blockAddress;

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);

    iNumber = getINumber((char*)parsedPath.back().c_str(), workingDirectory);
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



///function deletes bytes from the end of a given file
///parameters: path to file, number of bytes to delete
void VirtualDisk::deleteBytes(std::string path, unsigned int nBytesToDelete)
{
    short int iNumber;
    uint32_t oldFileSize;
    uint32_t newFileSize;
    uint16_t nBlocksToFree = 0;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    uint16_t blockAddress;

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);

    iNumber = getINumber((char*)parsedPath.back().c_str(), workingDirectory);
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



///function prints disk usage info
void VirtualDisk::printDiskUsageInfo()
{
    int nInodesTotal = nInodeBlocks * BLOCK_SIZE / I_NODE_SIZE;
    int nDataBlocksTotal = nBlocks - firstDataIndex;
    int nInodesInUse = 0;
    int nDataBlocksInUse = 0;
    int sizeForUserDataTotal = nDataBlocksTotal * BLOCK_SIZE;
    int sizeForUserDataInUse = 0;
    uint16_t fileSize; ///auxiliary

    ///count i-nodes and size of user data in use
    for(int i = 0; i < nInodesTotal; ++i)
    {
        if(checkBitFromBitmap(iNodeBitmapIndex, i))
        {
            ++nInodesInUse;

            ///read size of file
            fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + i * I_NODE_SIZE + SIZE_OFFSET, SEEK_SET);
            fread(&fileSize, sizeof(fileSize), 1, vDiskFile);
            sizeForUserDataInUse += (int)fileSize;
        }
    }

    ///count data blocks in use
    for(int i = 0; i < nDataBlocksTotal; ++i)
        if(checkBitFromBitmap(dataBitmapIndex, i))
            ++nDataBlocksInUse;

    std::cout << "Usage of space (in bytes): " << sizeForUserDataInUse << "/" << sizeForUserDataTotal << "\n";
    std::cout << "Usage of data blocks: " << nDataBlocksInUse << "/" << nDataBlocksTotal << "\n";
    std::cout << "Usage of i-nodes: " << nInodesInUse << "/" << nInodesTotal << "\n";
}



///function lists current directory
void VirtualDisk::listDirectory()
{
    uint16_t blockAddress;
    uint16_t sizeOfDirectory;
    uint16_t entryINumber;
    uint16_t entryLinkCount;
    uint16_t entrySize;
    uint16_t directoryINumber = (uint16_t)currentDirectory;
    bool entryFileType;
    char* entryName = new char[DIRECTORY_NAME_SIZE];

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



///function creates new directory in location specified by given path
///parameters: path to new directory
void VirtualDisk::createNewDirectory(std::string path)
{
    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);
    createChildDirectory(workingDirectory, (char*)parsedPath.back().c_str());
}



///function changes current directory
///parameters: path to new current directory
void VirtualDisk::changeDirectory(std::string path)
{
    std::vector<std::string> parsedPath = parsePath(path);
    if(-1 != specifyWorkingDirectory(parsedPath, MODE_CD))
    {
        currentDirectory = workingDirectory;
        pathToCurrentDir = workingPath;
    }
}



///function prints path to current directory
void VirtualDisk::printPath()
{
    if(pathToCurrentDir.empty())                       ///root directory
        std::cout << "/";

    for(int i = 0; i < pathToCurrentDir.size(); ++i)   ///other directories
        std::cout << "/" << pathToCurrentDir[i];

    std::cout << "\n";
}



///function adds link to a given file
///parameters: path to existing file, path to new file
void VirtualDisk::addLink(std::string target, std::string linkName)
{
    short int iNumber;
    bool isDirectory;

    ///find i-number
    std::vector<std::string> parsedPathToTarget = parsePath(target);
    specifyWorkingDirectory(parsedPathToTarget, MODE_OTHER);
    iNumber = getINumber((char*)parsedPathToTarget.back().c_str(), (uint16_t)workingDirectory);
    if(-1 == iNumber)
    {
        std::cerr << "No such file exists!\n";
        return;
    }

    ///check if it is a directory
    fseek(vDiskFile, firstINodeIndex * BLOCK_SIZE + iNumber * I_NODE_SIZE + IS_DIRECTORY_OFFSET, SEEK_SET);
    fread(&isDirectory, sizeof(isDirectory), 1, vDiskFile);
    if(isDirectory)
    {
        std::cerr << "Given file is a directory (links to directories not allowed)!\n";
        return;
    }

    ///add to directory
    std::vector<std::string> parsedPathToNewLink = parsePath(linkName);
    specifyWorkingDirectory(parsedPathToNewLink, MODE_OTHER);
    addDirectoryEntry((short int)workingDirectory, iNumber, (char*)parsedPathToNewLink.back().c_str());
}



///function prints contents of a given file on console
///parameters: path to file to print on console
void VirtualDisk::printOnConsole(std::string path)
{
    unsigned char* buffer = new unsigned char [BLOCK_SIZE]; ///auxiliary buffer to store data
    uint16_t blockAddress;
    uint16_t countBlocks;
    uint16_t bytesUsedInLastBlock;
    short int iNumber;
    uint16_t expectedBytes;
    uint32_t fileSize;

    std::vector<std::string> parsedPath = parsePath(path);
    specifyWorkingDirectory(parsedPath, MODE_OTHER);

    iNumber = getINumber((char*)parsedPath.back().c_str(), workingDirectory);
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
            return;
        }
        std::cout << buffer;
    }
}
