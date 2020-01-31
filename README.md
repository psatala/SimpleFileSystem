# SimpleFileSystem
The aim of this short project was to create a simple file system which can be stored in a single file (virtual disk).
The architecture of the system has been specified in [Solution Concept](https://github.com/psatala/SimpleFileSystem/blob/master/%5BPL%5DSolutionConcept.pdf) (polish version only).

This project was created during the third semester of computer science studies for the Operating Systems course.
That required it to be written in C/C++ language.

## Available commands
* `ls` - list all files from current directory in list format
* `pwd` - print working directory
* `info` - print information about virtual disk's usage
* `cd PATH_TO_NEW_DIR` - change directory to one specified by PATH_TO_NEW_DIR
* `mkdir PATH_TO_NEW_DIR` - create new directory in location specified by PATH_TO_NEW_DIR
* `ucp PATH_TO_FILE_ON_YOUR_SYSTEM PATH_TO_FILE_LOCATION_ON_VIRTUAL_DISK` - copy file from your system to virtual disk
* `dcp PATH_TO_FILE_ON_VIRTUAL_DISK PATH_TO_FILE_LOCATION_ON_YOUR_SYSTEM` - copy file from virtual disk to your system
* `ab PATH_TO_FILE_ON_VIRTUAL_DISK COUNT_BYTES_TO_ADD` - add COUNT_BYTES_TO_ADD null bytes (`'\0'`) to file specified by PATH_TO_FILE_ON_VIRTUAL_DISK
* `db PATH_TO_FILE_ON_VIRTUAL_DISK COUNT_BYTES_TO_DELETE` - delete COUNT_BYTES_TO_DELETE bytes from the end of file specified by PATH_TO_FILE_ON_VIRTUAL_DISK
* `ln PATH_TO_TARGET_FILE PATH_TO_NEW_FILE` - create a link to file specified by PATH_TO_TARGET_FILE by creating file specified by PATH_TO_NEW_FILE
* `rm PATH_TO_FILE_TO_DELETE` - delete file specified by PATH_TO_FILE_TO_DELETE
* `cat PATH_TO_FILE_TO_PRINT` - print contents of file specified by PATH_TO_FILE_TO_PRINT to the console
* `exit` - close the application
