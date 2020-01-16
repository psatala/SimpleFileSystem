#include <iostream>
#include <unistd.h>

#define MAX_DISK_SIZE 128 * 1024 * 1024
#define BLOCK_SIZE 4096
#define AVERAGE_FILE_SIZE_IN_BLOCKS 2
#define N_FILES_PER_I_NODE_BLOCK 32


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
    newSize = min(newSize, MAX_DISK_SIZE);    ///resize if size bigger than allowed

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
    int vDiskSize = getVDiskSize(vDiskFile);
    int nBlocks = vDiskSize / BLOCK_SIZE;
    int freeBlocks = nBlocks - 2;
    int nInodeBlocks = freeBlocks / (N_FILES_PER_I_NODE_BLOCK * AVERAGE_FILE_SIZE_IN_BLOCKS + 1);
    /** free blocks = nBlocks - 2
        average file -> 2 blocks
        x - number of files
        each i-node: 128B
            nLinks = 8
            nBlockAddresses = 31
            8B * nLinks = 64B
            2B * nBlockAddresses = 62B (short ints!!!)
            2B other (e.g. isDirectory)

            maxFileSize = 124kB

            each i-node block -> 32 files
            32 * nInodeBlocks ~= (freeBlocks - nInodeBlocks) / averageFileSizeInBlocks
            64 nIB = f - nIB
            65 nIB = f
            nIB = f / 65

            maxDiskSize = 128MB <- calculated from size of data bitmap (4kB = 32kb, which gives 32768 data blocks, each 4kB, total 128MB),
            also limited by (unsigned) short int range (65536)
    **/

    int iNodeBitmapIndex = 0;
    int dataBitmapIndex = 1;
    int firstINodeIndex = 2;
    int firstDataIndex = nInodeBlocks + firstINodeIndex;

}
