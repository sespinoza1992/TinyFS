#include "TinyFS.h"
#include "libTinyFS.h"

static void output(char *str) {
   printf("---%s\n", str);
   puts("      Press Enter to continue...");
   getchar();
   fflush(stdin);
}

int main() {
   int status = -5;
   uchar buff[5] = "test";
   uchar buff2[6] = "test2";
   uchar buff3[DATA_SIZE * 2 + 1];
   uchar contents[1] = {0x42};
   fileDescriptor fd1, fd2;
   char mybuffer = 0;
   char value1 = 0x67;
   char value2 = 0x42;

   memset(buff3, 0x42, DATA_SIZE * 2 + 1);
   buff3[45] = 0x67;

   output("Let's create a couple disks with and without specifying a name.");

   printf(">Formatting disk \"disk0.disk\" with 2560 bytes of space\n");
   status = tfs_mkfs("disk0.disk", BLOCKSIZE * 10);
   if(!status)
      printf("   Successfully created File System in file \"disk0.disk\"\n\n");
   else
      puts("   Failed!\n");
   
   printf(">Formatting a disk with 2348 bytes of space\n");
   status = tfs_mkfs(NULL, 2348);
   if(!status)
      printf("   Successfully created File System in file \"tinyFSDisk\"\n\n");
   else
      puts("   Failed!\n");
   
   output("Now let's mount the first disk and do some file operations.");

   printf(">Mounting disk \"disk0.disk\"\n");
   status = tfs_mount("disk0.disk");
   if(!status)
      printf("   Successfully mounted File System from file \"disk0.disk\"\n\n");
   else
      puts("   Failed!\n");

   printf(">Opening File \"file1\"\n");
   status = tfs_openFile("file1");
   fd1 = status;
   if(status >= 0)
      printf("   Successfully opened file \"file1\"\n\n");
   else
      puts("   Failed!\n");

   printf(">Opening File \"file2\"\n");
   status = tfs_openFile("file2");
   fd2 = status;
   if(status >= 0)
      printf("   Successfully opened file \"file2\"\n\n");
   else
      puts("   Failed!\n");

   output("Let's check the timestamps");
   tfs_readFileInfo(fd1);
   tfs_readFileInfo(fd2);

   output("Now let's write to file1");
   printf(">Filling File \"file1\" with 505 values of 0x42, with value 0x67 at byte 45\n\n");
   status = tfs_writeFile(fd1, (char *)buff3, DATA_SIZE * 2 + 1);
   if(!status)
      printf("   Successfully wrote data to file \"file1\"\n\n");
   else
      puts("   Failed!\n");
   
   output("Let's check the timestamps again");
   tfs_readFileInfo(fd1);
   tfs_readFileInfo(fd2);

   output("Let's seek to byte 45 to make sure 0x67 is there");
   puts(">Seeking to byte 45 of File \"file1\"");
   status = tfs_seek(fd1, 45);
   if(!status)
      printf("   Successfully seeked to byte 45 of file \"file1\"\n\n");
   else
      puts("   Failed!\n");

   output("Let's read byte 45 now");
   puts(">Reading byte 45 of file \"file1\"");
   status = tfs_readByte(fd1, &mybuffer);
   if(!status && mybuffer == value1)
      printf("   Successfully read byte 45 of file \"file1\"\n   Value Obtained: %#2X\n\n", mybuffer);
   else
      puts("   Failed!\n");

   output("Now let's seek to a different location to make sure is it 0x42");
   puts(">Seeking to byte 23 of File \"file1\"");
   status = tfs_seek(fd1, 23);
   if(!status)
      printf("   Successfully seeked to byte 23 of file \"file1\"\n\n");
   else
      puts("   Failed!\n");

   output("Now, let's read the value here");
   puts(">Reading byte 23 of file \"file1\"");
   status = tfs_readByte(fd1, &mybuffer);
   if(!status && mybuffer == value2)
      printf("   Successfully read byte 23 of file \"file1\"\n   Value Obtained: %#2X\n\n", mybuffer);
   else
      puts("   Failed!\n");

   output("Let's check the timestamps again");
   tfs_readFileInfo(fd1);
   tfs_readFileInfo(fd2);

   output("Now let's list the files we have");

   tfs_readdir();

   output("Let's delete a file then list all files again");
   puts(">Deleting File \"file1\"");
   status = tfs_deleteFile(fd1);
   if(!status)
      printf("   Successfully deleted file \"file1\"\n\n");
   else
      puts("   Failed!\n");

   tfs_readdir();

   output("Let's check the timestamps again");
   tfs_readFileInfo(fd2);

   output("Let's unmount the current disk");
   puts(">unmounting disk \"disk0.disk\"");
   status = tfs_unmount();
   if(!status)
      printf("   Successfully unmounted disk \"disk0.disk\"\n\n");
   else
      puts("   Failed!\n");
   
   return 0;
}
