#include "libTinyFS.h"

static int checksuperblock(int disknum) {
   uchar block[BLOCKSIZE] = {0};
   
   if(readBlock(disknum, SUPERBLOCK_ADDR, block)) {
      fprintf(stderr, "Superblock Failed Reading\n");
      return READ_ERROR;
   }
   
   if(block[0] != SUPERBLOCK || block[2] != ROOT_ADDR || block[3] != VALID) {
      fprintf(stderr, "Superblock Failed Check\n");
      return CORRUPT_FS;
   }

   return 0;
}

static int checkroot(int disknum) {
   uchar block[BLOCKSIZE] = {0};
   if(readBlock(disknum, ROOT_ADDR, block)) {
      fprintf(stderr, "Superblock Failed Reading\n");
      return READ_ERROR;
   }
   if(block[0] != INODE || block[2] != NULL_ADDR || block[3] != INVALID) {
      fprintf(stderr, "Root Failed Check\n");
      return CORRUPT_FS;
   }
   if(block[4] != 'r' || block[5] != 'o' || block[6] != 'o' || block[7] != 't') {
      fprintf(stderr, "Root Failed Check\n");
      return CORRUPT_FS;
   }
   if(block[8] || block[9] || block[10] || block[11]) {
      fprintf(stderr, "Root Failed Check\n");
      return CORRUPT_FS;
   }
   
   return 0;
}

static int checkmagicnum(int disknum) {
   int loop = 0;
   int numblocks = (getSize(disknum) - 1)/BLOCKSIZE + 1;
   uchar block[BLOCKSIZE] = {0};

   while(loop < numblocks) {
      if(readBlock(disknum, loop, block)) {
      fprintf(stderr, "Superblock Failed Reading\n");
         return READ_ERROR;
      }
      if(block[1] != MAGIC_NUM) {
         fprintf(stderr, "Failed MagicNumber Check\n");
         return CORRUPT_FS;
      }
      loop++;
   }
   return 0;
}

static int checkusedblock(uchar disknum, uchar blocknum) {
   uchar block[BLOCKSIZE] = {0};

   readBlock(disknum, blocknum, block);
   if(block[0] != SUPERBLOCK && block[0] != INODE && block[0] != FILE_EXTENT)
      return CORRUPT_FS;
   return 0;
}

static int checkfreeblock(uchar disknum, uchar blocknum) {
   uchar block[BLOCKSIZE] = {0};

   readBlock(disknum, blocknum, block);
   if(block[0] != FREE_BLOCK)
      return CORRUPT_FS;
   return 0;
}

static int checkbitmap(int disknum) {
   int outer = 0;
   int inner = 0;
   uchar bitmap[BITMAP_SIZE] = {0};
   int numblocks = (getSize(disknum) - 1) / BLOCKSIZE + 1;
   int numbytes = (numblocks - 1) / BITS_PER_BYTE + 1;

   getBitmap(disknum, bitmap);

   while(outer < BITMAP_SIZE && outer < numblocks) {
      while(inner < BITS_PER_BYTE) {
         if(GETBIT(bitmap[outer], (7 - inner))) {
            if(checkusedblock(disknum, outer * BITS_PER_BYTE + inner))
               return CORRUPT_FS;
         }
         else {
            if(checkfreeblock(disknum, outer * BITS_PER_BYTE + inner))
               return CORRUPT_FS;
         }
         inner++;
      }
      inner = 0;
      outer++;
   }

   return 0;
}

static int namecmp(char *name, int disknum, uchar blocknum) {
   uchar block[BLOCKSIZE];
   char filename[MAX_NAME_SIZE] = {'\0'};

   readBlock(disknum, blocknum, block);
   memcpy(filename, block + 4, MAX_NAME_SIZE);

   return strcmp(name, filename);
}

/* Checks FS for Integrity */
int checkfs(int disknum) {
   if(checksuperblock(disknum) || checkroot(disknum) || checkmagicnum(disknum))
      return CORRUPT_FS;

   return 0;
}

int nextFreeBlock(int disknum, int skip) {
   uchar block[BLOCKSIZE] = {0};
   uchar addr = 0;
   int outer = BITMAP_FIRST_ADDR;
   int inner;
   
   readBlock(disknum, SUPERBLOCK_ADDR, block);
   while(outer < BLOCKSIZE) {
      addr = block[outer];
      for(inner = 7; inner >= 0; inner--) {
         if(!GETBIT(addr, inner)) {
            if(!skip--)
               return (outer - BITMAP_FIRST_ADDR) * sizeof(uchar) + (7 - inner);
         }
      }
      outer++;
   }
   return ROOT_DIRECTORY_FULL;
}

int nextRootAddrIndex(int disknum) {
   uchar block[BLOCKSIZE] = {0};
   uchar addr = 0;
   int loop = ROOT_FIRST_ADDR;
   
   readBlock(disknum, ROOT_ADDR, block);
   while(loop < BLOCKSIZE) {
      addr = block[loop];
      if(!addr)
         return loop - 12;
      loop++;
   }
   return ROOT_DIRECTORY_FULL;
}

int getInodeBlock(char *name, int disknum) {
   uchar block[BLOCKSIZE];
   uchar addr = 0;
   int loop = ROOT_FIRST_ADDR;

   readBlock(disknum, ROOT_ADDR, block);
   while(loop < BLOCKSIZE) {
      addr = block[loop];
      if(addr) {
         if(!namecmp(name, disknum, addr))
            return addr;
      }
      loop++;
   }
   return FILE_NOT_FOUND;
}

/* Fills bitmap with BITMAP_SIZE bytes of bitmap found in superblock */
void getBitmap(int disknum, uchar *bitmap) {
   uchar block[BLOCKSIZE] = {0};

   readBlock(disknum, SUPERBLOCK_ADDR, block);
   memcpy(bitmap, block + 4, BITMAP_SIZE);
}

