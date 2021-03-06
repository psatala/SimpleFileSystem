///Name: Defines.h
///Purpose: define constants specifying disk features




#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED


///disk defines
#define BLOCK_SIZE 4096
#define MIN_DISK_SIZE 3 * BLOCK_SIZE
#define MAX_DISK_SIZE 128 * 1024 * 1024
#define AVERAGE_FILE_SIZE_IN_BLOCKS 2
#define MAX_FILE_SIZE_IN_BLOCKS 56
#define N_FILES_PER_I_NODE_BLOCK 32
#define DEFAULT_NAME "vDisk.vdf"

///i-node defines
#define I_NODE_SIZE 128
#define DATA_OFFSET 0
#define NAMES_OFFSET 112
#define OTHER_OFFSET 120
#define SIZE_OFFSET 120
#define LINK_COUNT_OFFSET 124
#define IS_DIRECTORY_OFFSET 126
#define NAME_SIZE 8
#define ADDRESS_SIZE 2

///directory defines
#define DIRECTORY_SIZE BLOCK_SIZE
#define DIRECTORY_ENTRY_SIZE 16
#define DIRECTORY_I_NUMBER_OFFSET 0
#define DIRECTORY_NAME_OFFSET 2
#define DIRECTORY_NAME_SIZE 14
#define DIRECTORY_MAX_ENTRIES BLOCK_SIZE / DIRECTORY_ENTRY_SIZE

///other
#define BYTE_SIZE 8
#define FREE 0
#define USED 1
#define MODE_CD 1
#define MODE_OTHER 2


#endif // DEFINES_H_INCLUDED
