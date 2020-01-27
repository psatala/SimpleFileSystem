#ifndef COMMANDLINEINTERPRETER_H_INCLUDED
#define COMMANDLINEINTERPRETER_H_INCLUDED

#include "VirtualDisk.h"


class CommandLineInterpreter
{
    VirtualDisk *vDisk;

    void printIncentive();
    std::vector<std::string> parseCommand();
    int interpretCommand(std::vector<std::string> parsedCommand);
    int checkArgumentCount(int argCount, int minCount, int maxCount);
public:
    CommandLineInterpreter(char* vDiskFileName = DEFAULT_NAME, int vDiskSize = -1);
    void run();
};



CommandLineInterpreter::CommandLineInterpreter(char* vDiskFileName, int vDiskSize)
{
    vDisk = new VirtualDisk(vDiskFileName, vDiskSize);
    run();
}



void CommandLineInterpreter::run()
{
    while(1)
    {
        printIncentive();
        if(-1 == interpretCommand(parseCommand()))
            break;
    }
}


void CommandLineInterpreter::printIncentive()
{
    std::cout << "Virtual_Disk$ ";
}


std::vector<std::string> CommandLineInterpreter::parseCommand()
{
    std::string command;
    std::string delimeter = " ";
    std::string token;
    std::vector<std::string> parsedCommand;
    size_t pos = 0;

    std::getline(std::cin, command);
    while((pos = command.find(delimeter)) != std::string::npos)
    {
        token = command.substr(0, pos);
        parsedCommand.push_back(token);
        command.erase(0, pos + delimeter.length());
    }
    parsedCommand.push_back(command);

    return parsedCommand;
}



int CommandLineInterpreter::interpretCommand(std::vector<std::string> parsedCommand)
{
    int returnValue = 0;

    if(parsedCommand.empty())
        return 0;

    if("ls" == parsedCommand[0])                                                     ///ls command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 1, 1))
            vDisk->listDirectory();
    }
    else if("pwd" == parsedCommand[0])                                               ///pwd command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 1, 1))
            vDisk->printPath();
    }
    else if("info" == parsedCommand[0])                                              ///info command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 1, 1))
            vDisk->printDiskUsageInfo();
    }
    else if("cd" == parsedCommand[0])                                                ///cd command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 2, 2))
            vDisk->changeDirectory(parsedCommand[1]);
    }
    else if("mkdir" == parsedCommand[0])                                             ///mkdir command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 2, 2))
            vDisk->createNewDirectory(parsedCommand[1]);
    }
    else if("ucp" == parsedCommand[0])                                               ///ucp command - up copy
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 3, 3))
            vDisk->copyToVDisk((char*)parsedCommand[1].c_str(), parsedCommand[2]);
    }
    else if("dcp" == parsedCommand[0])                                               ///dcp command - down copy
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 3, 3))
            vDisk->copyFromVDisk(parsedCommand[1], (char*)parsedCommand[2].c_str());
    }
    else if("ab" == parsedCommand[0])                                                ///ab command - add bytes
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 3, 3))
            vDisk->addBytes(parsedCommand[1], (unsigned int)stoi(parsedCommand[2]));
    }
    else if("db" == parsedCommand[0])                                                ///db command - delete bytes
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 3, 3))
            vDisk->deleteBytes(parsedCommand[1], (unsigned int)stoi(parsedCommand[2]));
    }
    else if("ln" == parsedCommand[0])                                                ///ln command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 3, 3))
            vDisk->addLink(parsedCommand[1], parsedCommand[2]);
    }
    else if("rm" == parsedCommand[0])                                                ///rm command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 2, 2))
            vDisk->deleteFile(parsedCommand[1]);
    }
    else if("cat" == parsedCommand[0])                                               ///cat command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 2, 2))
            vDisk->printOnConsole(parsedCommand[1]);
    }
    else if("exit" == parsedCommand[0])                                              ///exit command
    {
        if(-1 != checkArgumentCount(parsedCommand.size(), 1, 1))
        {
            returnValue = -1;
            delete vDisk;
        }
    }
    else
        std::cerr << parsedCommand[0] << ": command not found!\n";

    return returnValue;
}



int CommandLineInterpreter::checkArgumentCount(int argCount, int minCount, int maxCount)
{
    int returnValue = 0;

    if(argCount < minCount)
    {
        std::cerr << "Too few arguments for this command!\n";
        returnValue = -1;
    }
    else if(argCount > maxCount)
    {
        std::cerr << "Too many arguments for this command!\n";
        returnValue = -1;
    }

    return returnValue;
}


#endif // COMMANDLINEINTERPRETER_H_INCLUDED
