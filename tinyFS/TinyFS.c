#include "TinyFS.h"

static int mount = INVALID;
static tfile table[MAX_NUM_FILES];

static int isLEndian() {
   int end = 0x01;
   return *((uchar *)(&end));
}

static void initFD() {
   int loop = 0;

   memset(table, 0, sizeof(tfile) * MAX_NUM_FILES);
   for(loop = 0; loop < MAX_NUM_FILES; loop++) {
      table[loop].valid = INVALID;
   }
}

static int getFileSize(uchar *buffer) {
   int temp;

   memcpy(&temp, buffer + 13, 4);
   if(isLEndian())
      temp = SWAP_ENDIAN_INT(temp);

   return temp;
}

static void setBitmap(uchar *bitmap, uchar blocknum, blockstate state) {
   int index = blocknum/BITS_PER_BYTE;
   int bit = 7 - blocknum % BITS_PER_BYTE;

   if(state == USED) {
      bitmap[index] = SETBIT(bitmap[index], bit);
   }
   else
      bitmap[index] = CLRBIT(bitmap[index], bit);
}

/* Data MUST be of size BITMAP_SIZE */
static void updateBitmap(uchar *bitmap) {
   uchar block[BLOCKSIZE] = {0};

   readBlock(mount, SUPERBLOCK_ADDR, block);
   memcpy(block + 4, bitmap, BITMAP_SIZE);
   writeBlock(mount, SUPERBLOCK_ADDR, block);
}

static int getrootindex(uchar blocknum) {
   uchar block[BLOCKSIZE];
   uchar index = 0;

   readBlock(mount, ROOT_ADDR, block);
   while((index + 12) < BLOCKSIZE) {
      if(block[index + 12] == blocknum)
         return index;
      index++;
   }
   return FILE_NOT_FOUND;
}

/* Index 0 is first byte of inode pointers in root inode */
static void updateroot(uchar blocknum, uchar index) {
   uchar block[BLOCKSIZE];

   index += 12;
   readBlock(mount, ROOT_ADDR, block);
   block[index] = blocknum;
   writeBlock(mount, ROOT_ADDR, block);
}

static uchar *makeinode(uchar fileaddr, char *filename, uchar *block, int size,
                         uchar usedblocks) {
   
   memset(block, 0x00, BLOCKSIZE);
   block[0] = INODE;
   block[1] = MAGIC_NUM;
   block[2] = fileaddr;
   if(fileaddr)
      block[3] = VALID;
   else
      block[3] = INVALID;
   strncpy((char *)block + 4, filename, MAX_NAME_SIZE);
   block[12] = usedblocks;
   if(isLEndian())
      size = SWAP_ENDIAN_INT(size);
   memcpy(block + 13, &size, 4);
   return block;
}

/* Data MUST be of size DATA_SIZE */
static uchar *makedatablock(uchar nextblockaddr, uchar *data, uchar *block) {
   memset(block, 0x00, BLOCKSIZE);
   block[0] = FILE_EXTENT;
   block[1] = MAGIC_NUM;
   block[2] = nextblockaddr;
   if(nextblockaddr == NULL_ADDR)
      block[3] = INVALID;
   else
      block[3] = VALID;
   memcpy(block + 4, data, DATA_SIZE);
   return block;
}

static uchar *makefreeblock(uchar *block) {
   memset(block, 0x00, BLOCKSIZE);
   block[0] = FREE_BLOCK;
   block[1] = MAGIC_NUM;
   block[3] = INVALID;
   return block;
}

static uchar *makesuperblock(uchar *bitmap, uchar *block) {
   memset(block, 0x00, BLOCKSIZE);
   block[0] = SUPERBLOCK;
   block[1] = MAGIC_NUM;
   block[2] = ROOT_ADDR;
   block[3] = VALID;
   memcpy(block + 4, bitmap, BITMAP_SIZE);
   return block;
}

static fileDescriptor createFile(char *name) {
   fileDescriptor file;
   int i;
   int rootIndex;
   int inodeblock = nextFreeBlock(mount, 0);
   int datablock = nextFreeBlock(mount, 1);
   uchar buf[BLOCKSIZE];
   uchar junk[BLOCKSIZE] = {0};
   uchar bitmap[BITMAP_SIZE];

   getBitmap(mount, bitmap);
   setBitmap(bitmap, inodeblock, USED);
   setBitmap(bitmap, datablock, USED);
   updateBitmap(bitmap);
   // update root inode iterate
   // through setting first unused
   // block to 2 counting up from there
   rootIndex = nextRootAddrIndex(mount);
   if (rootIndex == ROOT_DIRECTORY_FULL)
      return ROOT_DIRECTORY_FULL;
 
   // Set address of file inode next
   // in open space of root inode
   
   // Make inode
   makeinode(datablock, name, buf, 0, 1);
   writeBlock(mount, inodeblock, buf);
   updateroot(inodeblock, rootIndex);

   // Make datablock
   makedatablock(NULL_ADDR, junk, buf);
   writeBlock(mount, datablock, buf);
   
   // Allocate a spot in process file table
   for (i = 0; i < MAX_NUM_FILES; i++) {
      if (table[i].valid == INVALID) {
         file = i;
         table[i].blocknum = datablock;
         table[i].pos = 0;
         table[i].valid = VALID;
         strncpy(table[i].name, name, MAX_NAME_SIZE);
         table[i].current_block = datablock;
         return file;
      }
   }

   return ROOT_DIRECTORY_FULL;
}

static uchar *initsuperblock(uchar *block) {
   uchar bitmap[BITMAP_SIZE] = {DEFAULT_FIRST_BITMAP_BYTE};
   
   assert(!bitmap[1] && !bitmap[BITMAP_SIZE - 1]);
   makesuperblock(bitmap, block);
   return block;
}

static time_t getTime(uchar inodenum, timestamp ts) {
   time_t timet = 0;
   uchar inode[BLOCKSIZE] = {0};
   uchar index = 0;

   readBlock(mount, inodenum, inode);

   if(ts == CREATED)
      index = CREATION_INDEX;
   else if(ts == MODIFIED)
      index = MOD_INDEX;
   else if(ts == ACCESSED)
      index = ACCESS_INDEX;

   memcpy(&timet, inode + index, sizeof(time_t));
   
   if(isLEndian())
      timet = SWAP_ENDIAN_LONG(timet);

   return timet;
}

static void updateTime(uchar inodenum, timestamp ts) {
   time_t timet = time(NULL);
   uchar inode[BLOCKSIZE] = {0};
   uchar index = 0;

   if(isLEndian())
      timet = SWAP_ENDIAN_LONG(timet);

   readBlock(mount, inodenum, inode);

   if(ts == CREATED)
      index = CREATION_INDEX;
   else if(ts == MODIFIED)
      index = MOD_INDEX;
   else if(ts == ACCESSED)
      index = ACCESS_INDEX;
   else {
      fprintf(stderr, "Invalid argument for arg 2 of updateTime\n");
      close(-1);
   }

   memcpy(inode + index, &timet, sizeof(time_t));
   writeBlock(mount, inodenum, inode);
}

int tfs_mkfs(char *filename, int nBytes) {
   int addr = 2;
   int disknum = INVALID;
   int blocknum = (nBytes - 1)/BLOCKSIZE + 1;
   char root[8] = {'r','o','o','t'};
   uchar block[BLOCKSIZE] = {0};

   if(!filename || !strcmp(filename, "")) 
      filename = DEFAULT_DISK_NAME;
   disknum = openDisk(filename, nBytes);
   if(disknum == -1)
      return OPEN_FAILURE;

   //set super-block
   initsuperblock(block);
   writeBlock(disknum, SUPERBLOCK_ADDR, block);

   //set root inode
   makeinode(NULL_ADDR, root, block, 0, 0);
   writeBlock(disknum, ROOT_ADDR, block);

   //set rest of blocks to free
   makefreeblock(block);
   for(addr = 2; addr < (MAX_NUM_FREE_BLOCKS + 2) && addr < blocknum; addr++) {
      writeBlock(disknum, addr, block);
   }

   return 0;
}

int tfs_mount(char *filename) {
   if(mount != INVALID) {
      return OPEN_FAILURE;
   }
   
   mount = findFile(filename);
   if(mount == -1) {
      mount = INVALID;
      return OPEN_FAILURE;
   }
   assert(mount >= 0);

   if(checkfs(mount) == CORRUPT_FS)
      return CORRUPT_FS;

   initFD();

   return mount;
}

int tfs_unmount(void) {
   if(mount == INVALID)
      return DISK_CLOSE_FAILURE;
   closeDisk(mount);
   mount = INVALID;
   
   return 0;
}

fileDescriptor tfs_openFile(char *name) {
   fileDescriptor file;
   int inodenum = 0;
   int i = 0;

   while (i < MAX_NUM_FILES) {
      if (table[i].valid == VALID && table[i].name == name){
         file = i;
         break;
      }
      i++;
   }
   file = i;
   if (file == MAX_NUM_FILES) {
      file = createFile(name);
   }
   inodenum = getInodeBlock(name, mount);
   updateTime(inodenum, CREATED);
   updateTime(inodenum, MODIFIED);
   updateTime(inodenum, ACCESSED);
   return file;
}



int tfs_closeFile(fileDescriptor FD) {
   table[FD].valid = INVALID;
   return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {   
   uchar data[BLOCKSIZE] = {0};
   uchar bitmap[BITMAP_SIZE] = {0};
   uchar inode[BLOCKSIZE] = {0};
   char towrite[BLOCKSIZE] = {0};
   int inodeblock = getInodeBlock(table[FD].name, mount);
   int errorCheck;
   int writes = (size - 1)/DATA_SIZE + 1;
   int blocks = writes;
   int blocksused;
   int freeblocksneeded;
   int i = 0;
   int copy;
   int currentblock;
   int nextblockaddr;
   int tempsize = size;


   if (table[FD].valid == INVALID)
      return WRITE_ERROR;

   updateTime(inodeblock, ACCESSED);
   
   readBlock(mount, inodeblock, inode);
   blocksused = inode[12];

   inode[12] = blocks;
   if(isLEndian())
      tempsize = SWAP_ENDIAN_INT(tempsize);
   memcpy(inode + 13, &tempsize, 4);

   freeblocksneeded = blocks - blocksused;
   if(freeblocksneeded > 0) {
      if(nextFreeBlock(mount, freeblocksneeded - 1) == ROOT_DIRECTORY_FULL) {
         fprintf(stderr, "Could not guarantee enough space for data\n");
         return ROOT_DIRECTORY_FULL;
      }
   }


   currentblock = inode[2];

   getBitmap(mount, bitmap);
   while (writes > 0) {
      setBitmap(bitmap, currentblock, USED);

      if(!blocksused--) {
         if(writes != 1) {
            nextblockaddr = nextFreeBlock(mount, i++);
            assert(nextblockaddr != ROOT_DIRECTORY_FULL);
         }
         else
            nextblockaddr = NULL_ADDR;
      }
      else {
         readBlock(mount, currentblock, data);
         if(blocksused) {
            nextblockaddr = data[2];
         }
         else
            nextblockaddr = nextFreeBlock(mount, i++);
      }

      if(size < DATA_SIZE)
         copy = size;
      else
         copy = DATA_SIZE;
      memcpy(towrite, buffer, copy);
      makedatablock(nextblockaddr, (uchar *)towrite, data);
      memset(towrite, 0x00, BLOCKSIZE);
      buffer += copy;
      size -= copy;

      errorCheck = writeBlock(mount, currentblock, data);
      if (errorCheck == WRITE_ERROR || errorCheck == OPEN_FAILURE
          || errorCheck == CLOSED_DISK_FAILURE)
         return errorCheck;

      writes--;
      currentblock = nextblockaddr;
   }
   updateBitmap(bitmap);
   // Set file pointer to 0
   table[FD].pos = 0;

   writeBlock(mount, inodeblock, inode);
   updateTime(inodeblock, MODIFIED);

   return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
   uchar bitmap[BLOCKSIZE];
   uchar block[BLOCKSIZE];
   uchar blank[BLOCKSIZE];
   uchar currentblock = getInodeBlock(table[FD].name, mount);
   uchar nextblock;
   int valid = VALID; 
   int index = getrootindex(currentblock);
   
   if(index == FILE_NOT_FOUND)
      return FILE_NOT_FOUND;
   
   makefreeblock(blank);
   getBitmap(mount, bitmap);

   while(currentblock != NULL_ADDR && valid == VALID) {
      readBlock(mount, currentblock, block);
      nextblock = block[2];
      valid = block[3];
      setBitmap(bitmap, currentblock, FREE);
      writeBlock(mount, currentblock, blank);
      currentblock = nextblock;
   }
   
   updateBitmap(bitmap);
   
   updateroot(NULL_ADDR, index);

   table[FD].blocknum = NULL_ADDR;
   table[FD].pos = 0;
   table[FD].valid = INVALID;
   memset(table[FD].name, '\0', MAX_NAME_SIZE + 1);
   table[FD].current_block = NULL_ADDR;

   return 0;
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
   uchar block[BLOCKSIZE];
   long pos;
     
   // Read in block from file at blocknum
   if (readBlock(mount, table[FD].current_block, block) != 0)
      return READ_ERROR;
	  
   // Set position to offset within block
   pos = table[FD].pos % DATA_SIZE + 4;
   
   // Copy single byte from block at offset to buffer
   memcpy(buffer, block + pos, sizeof(char));

   updateTime(getInodeBlock(table[FD].name, mount), ACCESSED);

   return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
   int inode;
   int offs = offset;
   int filesize;
   char block[BLOCKSIZE];
   char inodeblock[BLOCKSIZE];
   uchar cur_block;
   
   inode = getInodeBlock(table[FD].name, mount);
   readBlock(mount, inode, inodeblock);
   filesize = getFileSize((uchar *)inodeblock);
   cur_block = table[FD].blocknum;
   
   if (FD >= MAX_NUM_FILES || table[FD].valid == INVALID || offset >= filesize)
      return SEEK_ERROR;
   
   // Check if offset is greater than block size
   // if it is, traverse blocks until offset > DATA_SIZE
   // and update table[FD].blocknum accordingly.
   while (offset >= DATA_SIZE) {
      offset-= DATA_SIZE;
	  if (readBlock(mount, cur_block, block) != 0)
		return READ_ERROR;
	  cur_block = block[3];
   }
   table[FD].pos = offs;
   table[FD].current_block = cur_block;
   return 0;
}

int tfs_rename(fileDescriptor file, char *name) {
   int inode;
   uchar block[BLOCKSIZE] = {0};

   if (table[file].valid == INVALID)
      return FILE_NOT_FOUND;

   inode = getInodeBlock(table[file].name, mount);
   if (readBlock(mount, inode, block) != 0)
      return READ_ERROR;
   strncpy(block + 4, name, MAX_NAME_SIZE);
   strncpy(table[file].name, name, MAX_NAME_SIZE);
   writeBlock(mount, inode, block);

   updateTime(inode, MODIFIED);
   updateTime(inode, ACCESSED);

   return 0;
}

int tfs_readdir() {
   int rootndx;
   int loop = ROOT_FIRST_ADDR;
   uchar inode = 0;
   uchar block[BLOCKSIZE] = {0};
   uchar inodeblock[BLOCKSIZE] = {0};
   char name[9] = {0};

   printf("root (dir)\n");
   
   if (readBlock(mount, ROOT_ADDR, block) != 0)
      return READ_ERROR;

   while (loop < BLOCKSIZE) {
      inode = block[loop];
      if (inode) {
         if (readBlock(mount, inode, inodeblock) != 0)
            return READ_ERROR;
         strncpy(name, inodeblock + 4, MAX_NAME_SIZE);
         printf("  %s (file)", name);
      }
      loop++;
   }
   printf("\n");
   return 0;
}

void tfs_readFileInfo(fileDescriptor FD) {
   int inode = getInodeBlock(table[FD].name, mount);
   time_t rawcreated = getTime(inode, CREATED);
   time_t rawmod = getTime(inode, MODIFIED);
   time_t rawaccessed = getTime(inode, ACCESSED);
   tm *created;
   tm *mod;
   tm *accessed;
   char *str = NULL;

   printf("%s\n", table[FD].name);

   created = localtime(&rawcreated);
   str = asctime(created);
   printf("   Created: %s", str);

   mod = localtime(&rawmod);
   str = asctime(mod);
   printf("   Modified: %s", str);

   accessed = localtime(&rawaccessed);
   str = asctime(accessed);
   printf("   Accessed: %s\n", str);
}
