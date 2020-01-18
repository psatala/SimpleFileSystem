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

    return 0;
}
