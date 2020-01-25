#include "VirtualDisk.h"


using namespace std;



int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        cerr << "Name of virtual disk file not specified!\n";
        return 0;
    }

    VirtualDisk myDisk(argv[1]);
    myDisk.printDiskUsageInfo();
    myDisk.copyToVDisk("test2");
    myDisk.printDiskUsageInfo();
    myDisk.deleteBytes("test2", 10000);
    myDisk.printDiskUsageInfo();
    myDisk.copyFromVDisk("test2");
    myDisk.printDiskUsageInfo();

    return 0;
}
