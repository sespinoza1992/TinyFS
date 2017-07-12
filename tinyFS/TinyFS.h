#ifndef TINYFS_H
#define TINYFS_H

#include "libDisk.h"
#include "libTinyFS.h"
#include <time.h>
#include <assert.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256
/* Your program should use a 10240 Byte disk size giving you 40 blocks total. This is a default size. 
You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240 
/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define SUPERBLOCK 0x01
#define INODE 0x02
#define FILE_EXTENT 0x03
#define FREE_BLOCK 0x04
#define SUPERBLOCK_ADDR 0x00
#define ROOT_ADDR 0x01
#define NULL_ADDR 0x00
#define MAGIC_NUM 0x45
#define TRUE 1
#define FALSE 0
#define INVALID 0xAF
#define VALID 0x50
#define MAX_NUM_BLOCKS 256
#define MAX_NUM_FREE_BLOCKS 254
#define MAX_NUM_FILES 244
#define MAX_DISK_SIZE 65536
#define DATA_SIZE 252
#define BITMAP_SIZE 252
#define MAX_NAME_SIZE 8
#define DEFAULT_FIRST_BITMAP_BYTE 0xC0
#define ROOT_FIRST_ADDR 12
#define BITMAP_FIRST_ADDR 4
#define BITS_PER_BYTE 8
#define CREATION_INDEX 17
#define MOD_INDEX 25
#define ACCESS_INDEX 33

#define SWAP_ENDIAN_INT(x) \
((((x) & 0xFF000000) >> 24) |\
(((x) & 0x00FF0000) >> 8) |\
(((x) & 0x0000FF00) << 8) |\
(((x) & 0x000000FF) << 24))

#define SWAP_ENDIAN_LONG(x) \
((((x) & 0xFF00000000000000) >> 56) |\
(((x) & 0x00FF000000000000) >> 40) |\
(((x) & 0x0000FF0000000000) >> 24) |\
(((x) & 0x000000FF00000000) >> 8) |\
(((x) & 0x00000000FF000000) << 8) |\
(((x) & 0x0000000000FF0000) << 24) |\
(((x) & 0x000000000000FF00) << 40) |\
(((x) & 0x00000000000000FF) << 56))


typedef int fileDescriptor;
typedef unsigned char uchar;
typedef enum blockstate {FREE, USED} blockstate;
typedef enum timestamp {CREATED, MODIFIED, ACCESSED} timestamp;

/* Index of Files in Process File Table are the fileDescriptor numbers
      Ex: table[0] returns the tfile (metadata) of file number 0 */
typedef struct tfile {
   uchar blocknum;
   long pos;
   uchar valid;
   char name[9];
   uchar current_block;
} tfile;

/* Makes a blank TinyFS file system of size nBytes on the file specified by filename.
This function should use the emulated disk library to open the specified file, and upon
success, format the file to be mountable. This includes initializing all data to 0x00,
setting magic numbers, initializing and writing the superblock and inodes, etc. Must
return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *filename) mounts a TinyFS file system located within filename.
tfs_unmount(void) unmounts the currently mounted file system. As part of the mount
operation, tfs_mount should verify the file system is the correct type. Only one file
system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted
file system. Must return a specified success/error code. */
int tfs_mount(char *filename);

int tfs_unmount(void);

/* Opens a file for reading and writing on the currently mounted file system.
Creates a dynamic resource table entry for the file, and returns a file descriptor
(integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer buffer of size size, which represents an entire files content,
to the file system. Sets the file pointer to 0 (the start of file) when done.
Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file pointer
location and incrementing it by one upon success. If the file pointer is already at the
end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);

int tfs_rename(fileDescriptor file, char *name);

int tfs_readdir();

void tfs_readFileInfo(fileDescriptor FD);

#endif
