///Name: VirtualDisk.h
///Purpose: declare and describe VirtualDisk class - the most important class of the program




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

        maxFileSize = 124kB

        each i-node block -> 32 files

        maxDiskSize = 128MB <- calculated from size of data bitmap (4kB = 32kb, which gives 32768 data blocks, each 4kB, total 128MB),
        also limited by (unsigned) uint16_t range (65536)
**/


class VirtualDisk
{


/********************************************************************************************************************************************************************************************
 *                                                                           private attributes                                                                                             *
 ********************************************************************************************************************************************************************************************/

    FILE *vDiskFile;                           ///pointer to file on user system implementing virtual disk
    char* vDiskFileName;                       ///name
    int vDiskSize;                             ///size
    int nBlocks;                               ///total number of blocks
    int freeBlocks;                            ///number of blocks for i-node tables and user data
    int nInodeBlocks;                          ///number of blocks for i-node tables
    int iNodeBitmapIndex;                      ///index of i-node bitmap
    int dataBitmapIndex;                       ///index of data bitmap
    int firstINodeIndex;                       ///index of first i-node block
    int firstDataIndex;                        ///index of first data block

    int currentDirectory;                      ///current directory (usually given by the 'pwd' command)
    int workingDirectory;                      ///temporary current directory within a function
    std::vector<std::string> pathToCurrentDir; ///path to current directory
    std::vector<std::string> workingPath;      ///path to temporary current directory





/********************************************************************************************************************************************************************************************
 *                                                                           private methods                                                                                                *
 ********************************************************************************************************************************************************************************************/


    ///function opens file from user system be used for virtual disk implementation
    void openFile();



    ///function closes file from user system be used for virtual disk implementation
    void closeFile();



    ///function clears i-node and data bitmaps during virtual disk creation
    void prepareBitmaps();



    ///function sets virtual disk size
    ///parameters: new size of virtual disk (in bytes)
    void setVDiskSize(int newSize);



    ///function gets virtual disk size
    ///return value: size of virtual disk
    int getVDiskSize();



    ///function sets virtual disk parameters during virtual disk creation - based on disk size, calculate where bitmaps, i-node table and user data are
    void setVDiskParameters();



    ///function creates empty directory
    ///return value: i-number of created directory
    short int createEmptyDirectory();



    ///function creates root directory
    void createRootDirectory();



    ///function creates child directory
    ///parameters: i-number of parent directory, name of child directory
    void createChildDirectory(uint16_t directoryINumber, char* childName);



    ///function adds directory entry
    ///parameters: i-number of directory to add in. i-number of file to add, name of file to add
    void addDirectoryEntry(short int directoryINumber, short int iNumberToAdd, char* fileNameToAdd);



    ///function deletes directory entry
    ///parameters: i-number of directory to delete from, name of file to delete
    void deleteDirectoryEntry(short int directoryINumber, char* fileNameToDelete);



    ///function finds next free data block
    ///return value: index of first free data block
    short int findNextFreeBlock();



    ///function finds next free i-node
    ///return value: first free i-number
    short int findNextFreeInode();



    ///function changes data block status
    ///parameters: index of data block to change, new status (free or used)
    void changeBlockStatus(int blockId, bool newStatus);



    ///function changes i-node status
    ///parameters: i-number to change, new status (free or used)
    void changeINodeStatus(int iNodeId, bool newStatus);



    ///function checks status of a bit from a given bitmap
    ///parameters: id of bitmap (i-node or data block), id of entry on that bitmap
    ///return value: status of bit (free or used)
    bool checkBitFromBitmap(int bitmapId, int entryId);



    ///function gets i-number of given file from a given directory
    ///parameters: name of file, i-number of directory to search in
    ///return value: i-number of given file
    short int getINumber(char* fileName, uint16_t directoryINumber);



    ///function parses path - from string to vector
    ///parameters: path in string form
    ///return value: path in vector form
    std::vector<std::string> parsePath(std::string path);



    ///function interprets parsed path to specify working (temporary current) directory
    ///parameters: parsed path in vector form, mode of specifying (whether to interpret last element of parsed path or not)
    ///return value: -1 if could not resolve parsed path
    short int specifyWorkingDirectory(std::vector<std::string> parsedPath, int mode = MODE_CD);



    ///function increases link count of a given file
    ///parameters: i-number of file to increase link counter
    void increaseLinkCount(uint16_t fileINumber);



    ///function decreases link count of a given file
    ///parameters: i-number of file to decrease link counter
    void decreaseLinkCount(uint16_t fileINumber);






/********************************************************************************************************************************************************************************************
 *                                                                              public methods                                                                                              *
 ********************************************************************************************************************************************************************************************/



public:

    ///constructor
    ///parameters: name of virtual disk file, size of virtual disk file
    VirtualDisk(char* newVDiskFileName = DEFAULT_NAME, int diskSize = -1);



    ///destructor
    ~VirtualDisk();



    ///function copies file from user system to virtual disk
    ///parameters: name of file to copy from user system, path to target location
    void copyToVDisk(char* fileNameToCopy, std::string path);



    ///function copies file from virtual disk to user system
    ///parameters: path to file on virtual disk, name of target file on user system
    void copyFromVDisk(std::string path, char* fileNameToCopy);



    ///function deletes a file from virtual disk
    ///parameters: path to file to delete
    void deleteFile(std::string path);



    ///function adds null bytes to the end of given file
    ///parameters: path to file, number of bytes to add
    void addBytes(std::string path, unsigned int nBytesToAdd);



    ///function deletes bytes from the end of a given file
    ///parameters: path to file, number of bytes to delete
    void deleteBytes(std::string path, unsigned int nBytesToDelete);



    ///function prints disk usage info
    void printDiskUsageInfo();



    ///function lists current directory
    void listDirectory();



    ///function creates new directory in location specified by given path
    ///parameters: path to new directory
    void createNewDirectory(std::string path);



    ///function changes current directory
    ///parameters: path to new current directory
    void changeDirectory(std::string path);



    ///function prints path to current directory
    void printPath();



    ///function adds link to a given file
    ///parameters: path to existing file, path to new file
    void addLink(std::string target, std::string linkName);



    ///function prints contents of a given file on console
    ///parameters: path to file to print on console
    void printOnConsole(std::string path);



};




#endif // VIRTUALDISK_H_INCLUDED
