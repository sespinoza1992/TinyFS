#include "libDisk.h"
#include <assert.h>

static Disk *disk_list = NULL;
static Disk *curr = NULL;
static Disk *endptr = NULL;
static int open_disks = 0;

int openDisk(char *filename, int nBytes) {
   FILE *fd = NULL;
   int disk_num;
     
   if (nBytes > 0) {
      fd = fopen(filename, "w+b");
   }
   else {
      fd = fopen(filename, "rb");    
   }
   assert(fd);
   disk_num = createDisk(filename, nBytes, fd); 
   //printf("Open success!\n");
   return disk_num;
}

int readBlock(int disk, int bNum, void *block) {
   Disk *temp = NULL;
   FILE *fd = NULL;
   int read = 0;

   //fprintf(stderr, "Reading Block %d\n", bNum);
   if ((temp = findDisk(disk)) == NULL) {
      printf("Failed here!\n");
      return OPEN_FAILURE;
   }
   //fprintf(stderr, "Found disk \n");
   //fprintf(stderr, "File name of disk: %s\n", temp->name);
   if (temp->open == 0)
      return CLOSED_DISK_FAILURE;
   
   //fprintf(stderr, "Disk is open \n");
   fd = temp->file;
   if (fseek(fd, bNum * BLOCKSIZE, SEEK_SET) == 0) {
      
   //fprintf(stderr, "Seeked \n");
      read = fread(block, BLOCKSIZE, 1, fd);
   //fprintf(stderr, "Read %d elements from disk \n", read);
      if (read > 0)
         return 0;
   }
   return READ_ERROR;
}

int writeBlock(int disk, int bNum, void *block){
   Disk *temp;
   FILE *fd;
   int write;

   if ((temp = findDisk(disk)) == NULL)
      return OPEN_FAILURE;
   if (temp->open == 0)
      return CLOSED_DISK_FAILURE;

   fd = temp->file;
   if (fseek(fd, bNum * BLOCKSIZE, SEEK_SET) == 0) {
      write = fwrite(block, BLOCKSIZE, 1, fd);
      if (write > 0)
         return 0;
   }
   return WRITE_ERROR;
}

void closeDisk(int disk) {
   Disk *temp;

   if ((temp = findDisk(disk)) == NULL) {
      printf("Invalid disk to close\n");
      return;
   }
   if (fflush(temp->file) != 0)
      printf("Flushing data failed\n");

   temp->open = 0;
   fclose(temp->file);
   open_disks--;
}

int findFile(char *filename) {
   int disk_lookup = 0;
   curr = disk_list;
   while (curr) {
      if (strcmp(filename, curr->name) == 0)
         return disk_lookup;
      disk_lookup++;
      curr = curr->next;
   }
   // file not found
   return OPEN_FAILURE;
}

int createDisk(char *filename, int nBytes, FILE *fd) {
   Disk *add = calloc(1, sizeof(Disk));

   add->size = nBytes;
   add->name = calloc(strlen(filename) + 1, sizeof(char));
   strcpy(add->name, filename);
   add->open = 1;
   add->file = fd;

   if (disk_list == NULL) {
      disk_list = add;
   }
   else {
      endptr->next = add;
   }
   endptr = add;

   return open_disks++;
}

Disk *findDisk(int index) {
   curr = disk_list;
   while (index-- > 0) {
      if (curr)
         curr = curr->next;
      else {
         printf("Got here!\n");
         return NULL;
      }
   }
   if (curr) {
      //printf("curr points to something\n");
      //printf("curr size is %d\n", curr->size);
      //printf("curr name is %s\n", curr->name);
      //printf("curr open status is %d\n", curr->open);
   }
   //printf("Returning from finddisk!\n");
   return curr;
}

int getSize(int disknum) {
   Disk *disk = findDisk(disknum);
   return disk->size;
}
