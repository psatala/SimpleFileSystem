#include <iostream>
#include <unistd.h>

#define BLOCK_SIZE 4096

using namespace std;


void setVDiskSize(FILE* vDiskFile);
int getVDiskSize(FILE* vDiskFile);

void setVDiskParameters(FILE* vDiskFile);


int main(int argc, char** argv)
{
    char* vDiskFileName;
    FILE *vDiskFile;


/*********************************************************************
 *                             opening file                          *
 *********************************************************************/

    if(argc > 1)
        vDiskFileName = argv[1];
    else
    {
        cerr << "Name of virtual disk file not specified!\n";
        return 0;
    }

    if(access(vDiskFileName, F_OK) != -1)       ///file exists
        vDiskFile = fopen(vDiskFileName, "rb+");
    else                                        ///file does not exist
        vDiskFile = fopen(vDiskFileName, "wb+");

    if(vDiskFile == NULL)
    {
        cerr << "Could not open virtual disk file!\n";
        return 0;
    }


    setVDiskSize(vDiskFile);

    fclose(vDiskFile);
    return 0;
}



void setVDiskSize(FILE* vDiskFile)
{
    int newSize;
    cin >> newSize;

    newSize = newSize - newSize % BLOCK_SIZE; ///resize to be a multiply of block size

    fseek(vDiskFile, newSize - 1, SEEK_SET);
    fprintf(vDiskFile, "x");
}

int getVDiskSize(FILE* vDiskFile)
{
    fseek(vDiskFile, 0, SEEK_END);
    return ftell(vDiskFile);
}


void setVDiskParameters(FILE* vDiskFile)
{
    ///based on disk size, calculate where bitmaps, i-node table and user data are
    int vDiskSize = getVDiskSize(vDisk);
    int nBlocks = vDiskSize / BLOCK_SIZE;
    int nInodeBlocks =
    /** free blocks = nBlocks - 2
        average file -> 1 block
        x - number of files
        each i-node:
            nLinks = 8
            nBlockAddresses = 256
            8B * nLinks = 64B
            4B * nBlockAddresses = 1024B
            1B isDirectory
    **/

    int iNodeBitmapIndex = 0;
    int dataBitmapIndex = 1;
    int firstINodeIndex = 2;
    int firstDataIndex = nInodeBlocks + firstINodeIndex;

}
