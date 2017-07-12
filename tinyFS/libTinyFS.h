#ifndef LIBTINYFS_H
#define LIBTINYFS_H

#include "libDisk.h"
#include "TinyFS.h"
#include <time.h>

#define GETBIT(value,bit) \
(((value) >> (bit)) & 0x01)

#define SETBIT(value,bit) \
((value) | (1 << (bit)))

#define CLRBIT(value,bit) \
((value) & (~(1 << (bit))))

typedef unsigned char uchar;
typedef struct tm tm;

/* Checks FS on disk number disknum for integrity (proper superblock and root,
    magic number present on all blocks in second byte */
int checkfs(int disknum);

/* Returns address of the first free block after [skip] number of free
    blocks are skipped
      Ex. If skip is '1', return address of second free block
   Returns ROOT_DIRECTORY_FULL if there are no free blocks after [skip] number
    of free blocks */
int nextFreeBlock(int disknum, int skip);

/* Returns index of first unused inode pointer in root
    index 0 is the first pointer in root */
int nextRootAddrIndex(int disknum);
int getInodeBlock(char *name, int disknum);
void getBitmap(int disknum, uchar *bitmap);

#endif
