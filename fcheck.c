#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "xv6/include/types.h"
#include "xv6/include/fs.h"

#define BLOCK_SIZE (BSIZE)

#define ERROR_CODE 1
#define ROOTINO 1
#define T_UNALLOC 0

#define T_DIR 1
#define T_FILE 2
#define T_DEV 3

#define I_BUSY 0x1
#define I_VALID 0x2


int
main(int argc, char *argv[])
{
  int i,n,fsfd,j;
  char *addr;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;
  struct stat statb;

  if(argc < 2){
    fprintf(stderr, "Usage: sample fs.img ...\n");
    exit(ERROR_CODE);
  }


  fsfd = open(argv[1], O_RDONLY);
  if(fsfd < 0){
    fprintf(stderr, "image not found.\n");
    exit(ERROR_CODE);
  }

  if (fstat(fsfd, &statb) == -1) {
    perror("fstat");
    exit(ERROR_CODE);
  }

  printf("fs.img size: %jd\n", statb.st_size);
  /* Dont hard code the size of file. Use fstat to get the size */
  addr = mmap(NULL, statb.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
	  perror("mmap failed");
	  exit(1);
  }

  /* read the super block */
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);

  /* read the inodes */
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  printf("begin addr %p, begin inode %p , offset %ld \n", addr, dip, (char *)dip -addr);

  // read root inode
  printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // get the address of root dir 
  de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);

  // print the entries in the first block of root dir 

  n = dip[ROOTINO].size/sizeof(struct dirent);
  for (i = 0; i < n; i++,de++){
 	printf(" inum %d, name %s ", de->inum, de->name);
  	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);

    switch (dip[de->inum].type) {
      case T_DIR:
      case T_FILE:
      case T_DEV:
      case T_UNALLOC:
      break;
      default:
      fprintf(stderr, "ERROR: bad inode.\n");
      exit(ERROR_CODE);
    }

    if (dip[de->inum].type == T_UNALLOC) {
      continue;
    }

    for (j = 0; j < NDIRECT; j++) {
      /* If file size is 0 but blocks are allocated */
      if (dip[de->inum].size == 0 && dip[de->inum].addrs[j] != 0) {
        fprintf(stderr, "ERROR: bad direct address in inode.\n");
        exit(ERROR_CODE);
      }
      /* If direct block addr is outside fs img range */
      if (dip[de->inum].addrs[j] != 0 && (dip[de->inum].addrs[j] >= sb->size || dip[de->inum].addrs[j] <= 0)) {
        fprintf(stderr, "ERROR: bad direct address in inode.\n");
        exit(ERROR_CODE);
      }
    }
    if (dip[de->inum].addrs[j] != 0 && dip[de->inum].addrs[j] >= sb->size) {
      fprintf(stderr, "ERROR: bad indirect address in inode.\n");
      exit(ERROR_CODE);
    }
      
    
  }

//leave:
  munmap(addr, statb.st_size);
  exit(0);

}

