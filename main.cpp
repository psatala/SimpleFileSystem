#include "CommandLineInterpreter.h"
#include <string.h>

using namespace std;



int main(int argc, char** argv)
{
    int diskSize = -1;
    if(argc <= 1)
    {
        cerr << "Name of virtual disk file not specified!\n";
        return 0;
    }
    if(argc >= 3)
        diskSize = atoi(argv[2]);

    CommandLineInterpreter myCMD(argv[1], diskSize);
    /*VirtualDisk myDisk(argv[1], diskSize);
    myDisk.createNewDirectory("newDir");
    myDisk.listDirectory();
    myDisk.changeDirectory("newDir");
    myDisk.listDirectory();
    myDisk.createNewDirectory("../wololo");
    myDisk.changeDirectory("..");
    myDisk.copyToVDisk("wololo/test2");
    myDisk.listDirectory();
    myDisk.changeDirectory("wololo");
    myDisk.listDirectory();
    myDisk.printDiskUsageInfo();
    myDisk.changeDirectory("../newDir");
    myDisk.addLink("../wololo/test2", "../linkToTest2");
    myDisk.printPath();
    myDisk.changeDirectory("..");
    myDisk.listDirectory();
    myDisk.changeDirectory("wololo");
    myDisk.listDirectory();
    myDisk.createNewDirectory("foobar");
    myDisk.changeDirectory("foobar");
    myDisk.printPath();*/
    /*myDisk.copyToVDisk("test2");
    myDisk.listDirectory(0);
    myDisk.printDiskUsageInfo();
    myDisk.deleteBytes("test2", 10000);
    myDisk.printDiskUsageInfo();
    myDisk.addBytes("test2", 12000);
    myDisk.printDiskUsageInfo();
    myDisk.copyFromVDisk("test2");
    myDisk.listDirectory(0);
*/
    return 0;
}
