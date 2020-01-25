#include "VirtualDisk.h"
#include <string.h>

using namespace std;



int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        cerr << "Name of virtual disk file not specified!\n";
        return 0;
    }

    VirtualDisk myDisk(argv[1]);
    myDisk.listDirectory(0);
    /*myDisk.printDiskUsageInfo();
    myDisk.copyToVDisk("test2");
    myDisk.printDiskUsageInfo();
    myDisk.deleteBytes("test2", 10000);
    myDisk.printDiskUsageInfo();
    myDisk.addBytes("test2", 12000);
    myDisk.printDiskUsageInfo();
    myDisk.copyFromVDisk("test2");*/

    return 0;
}
