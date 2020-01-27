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

    return 0;
}
