/*
File System Checker
Author : Adarsh Ramesh Kumar , NetID: axr210179
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

#include "types.h"
#include "fs.h"

#define BLOCK_SIZE (BSIZE)


#define T_DIR 1
#define T_FILE 2
#define T_DEV 3



struct dinode *inodeStrct;
struct superblock *sbStrct;
int inodeBlockCount;
struct dirent *directEntryStruct;
char *mmapbuf;
char *bitmapAddr;
int bitmapBlockCount;
char *dataAddr;
int *dataBlock;


#define DE_NUM (inodeStrct[ROOTINO].size/sizeof(struct dirent))


//Function to check for rule 1
void rule_1_check()
{
    int i;
    struct dinode *inodetemp = inodeStrct;
    for(i = 0;i<sbStrct->ninodes;i++, inodetemp++)
    {
        if((inodetemp->type == 0) || (inodetemp->type == T_FILE) 
            || inodetemp->type == T_DIR ||inodetemp->type == T_DEV)
            continue;
        else
            fprintf(stderr, "ERROR: bad inode.\n");
            exit(1);
    }

}

//Function to check for rule 2
void rule_2_check()
{
    int i, j;
    
    struct dinode *inodetemp = inodeStrct;
    uint bitmapBlockEnd = BBLOCK(IBLOCK(sbStrct->ninodes -1), sbStrct->ninodes);
    //Loop through the inodes
    for(j=0;j<sbStrct->ninodes;j++, inodetemp++)
    {
        //Check for the direct block address
        for(i = 0;i<NDIRECT;i++)
        {
            //block is not used 
            if(inodetemp->addrs[i] == 0)
                continue;
            //block allocated should have a positive address less than the total block number
            if((inodetemp->addrs[i] > bitmapBlockEnd) && (inodetemp->addrs[i] < sbStrct->nblocks))
                continue;  
            //if the above conditions fail : error invalid address    
            fprintf(stderr, "ERROR: bad direct address in inode.\n");
            exit(1);
        }


        //Check for the indirect block address

        //check for valid indirect block start
        uint indirectBlockAddr = inodetemp->addrs[NDIRECT];
        
        //block not allocated :no point in further checks
        if(indirectBlockAddr == 0)
            continue;
        
        //block allocated should have a positive address less than the total block number
        if(((indirectBlockAddr) <= bitmapBlockEnd) || ((indirectBlockAddr) >= sbStrct->nblocks))
        {
            fprintf(stderr, "ERROR: bad indirect address in inode.\n");
            exit(1);
        }

        //indirect block start address
        uint *curIndirectAddr = (uint *) (mmapbuf + indirectBlockAddr * BLOCK_SIZE);

        for(i = 0;i<NINDIRECT;i++,curIndirectAddr++)
        {
            //block is not used
            //value of the pointer holds the address 
            if((*curIndirectAddr)  == 0)
                continue;
            
            //block allocated should have a positive address less than the total block number
            if(((*curIndirectAddr) > bitmapBlockEnd) && ((*curIndirectAddr) < sbStrct->nblocks))
                continue;
                
            //if the above conditions fail : error invalid address    
            fprintf(stderr, "ERROR: bad indirect address in inode.\n");
            exit(1);
        }
    }

}


void rule_3_check() 
{
    int i, n;
    bool parentItself = false, rootDir = false, rootDirInum = false;
    struct dirent *deTemp;
    
    struct dinode *inodeTemp = inodeStrct;
    
    //First inode not used
    inodeTemp++; 
    
    n = inodeTemp->size/sizeof(struct dirent);

    if (inodeTemp->addrs != 0) 
    {
      deTemp = (struct dirent *) (mmapbuf + (inodeTemp->addrs[0])*BLOCK_SIZE);
      if (deTemp != 0) 
      {
        rootDir = true;
        if (deTemp->inum == 1)
          rootDirInum = true;
        // iterate through dirents of the root inode
        for (i = 0; i < n; i++,deTemp++) 
        {
          if (strcmp(deTemp->name,"..") == 0 && deTemp->inum == 1)
          {
            parentItself = true;
            break;
          }
        }
      }
    }

    //Rule 3 : if parent and present directories not found
    if (!rootDir || !rootDirInum || !parentItself) {
      fprintf(stderr, "ERROR: root directory does not exist.\n");
      exit(1);
    }
}

void rule_4_check() {
  int i, j, k;
  bool presentFlag, parentFlag, presentCheck;
  struct dirent *deTemp;
  
  struct dinode *inodeTemp = inodeStrct;

  // iterate through all inodes
  for (i = 1; i < sbStrct->ninodes; i++, inodeTemp++) 
  {
    if (inodeTemp->addrs == 0) 
      continue;

    if (inodeTemp->type == 0)
      continue;

    if (inodeTemp->type == 1) 
    { 
      if (inodeTemp->addrs != 0) 
      {
        presentFlag = false;
        parentFlag = false;
        presentCheck = false;
        if (inodeTemp->type == 1) 
        { 
          if (inodeTemp->addrs != 0) 
          {
            // check direct blocks
            for (j = 0; j < NDIRECT; j++) 
            {
              if (inodeTemp->addrs[j] == 0) 
                continue;
              deTemp = (struct dirent *) (mmapbuf + (inodeTemp->addrs[j])*BLOCK_SIZE);
              if (deTemp != 0) 
              {
                // Loop through directory elements
                for (k = 0; k < DE_NUM; k++,deTemp++) 
                {
                  if (deTemp == 0 || deTemp->inum == 0)
                    continue;

                  if (strcmp(deTemp->name,".") == 0) 
                  {
                    presentFlag = true;
                    if (deTemp->inum == i - 1)
                      presentCheck = true;
                  }

                  if (strcmp(deTemp->name,"..") == 0)
                    parentFlag = true;

                  if (presentFlag && parentFlag && presentCheck)
                    break;
                }
              }
              if (presentFlag && parentFlag && presentCheck)
                break;
            }
            // check indirect blocks
            uint indAddr = inodeTemp->addrs[NDIRECT];
            if (indAddr == 0) 
            { 
              if (!presentFlag || !parentFlag || !presentCheck) 
              {
                fprintf(stderr, "ERROR: directory not properly formatted.\n");
                exit(1);
              } 
              else
                continue;
            }
            uint *indTemp = (uint *) (mmapbuf + indAddr * BLOCK_SIZE);
            for (j = 0; j < NINDIRECT; j++, indTemp++) 
            {
              if ((*indTemp) == 0) 
                continue;
              deTemp = (struct dirent *) (mmapbuf + (*indTemp)*BLOCK_SIZE);
              if (deTemp != 0) 
              {
                // Loop through directory entries
                for (k = 0; k < DE_NUM; k++,deTemp++) 
                {
                  if (deTemp == 0 || deTemp->inum == 0)
                    continue;
                  if (strcmp(deTemp->name,".") == 0) 
                  {
                    presentFlag = true;
                    if (deTemp->inum == i - 1)
                      presentCheck = true;
                  }

                  if (strcmp(deTemp->name,"..") == 0)
                    parentFlag = true;

                  if (presentFlag && parentFlag && presentCheck)
                    break;
                }
              }
              if (presentFlag && parentFlag && presentCheck)
                break;
            }
            if (!presentFlag || !parentFlag || !presentCheck) 
            {
              fprintf(stderr, "ERROR: directory not properly formatted.\n");
              exit(1);
            }
          }
        }
      }
    }
  }
}

void rule_5_check() 
{
  int i , j;
  struct dinode *inodeTemp = inodeStrct;

  uint inodeBlockCount = (sbStrct->ninodes/IPB) + 1;

  char* bitmapBlock = (char*) ((char *)inodeTemp + inodeBlockCount * BLOCK_SIZE);
  uint finalBitBlock = BBLOCK(IBLOCK(sbStrct->ninodes - 1), sbStrct->ninodes);
  char Mask[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  
  // iterate through all inodes
  for (i = 1; i < sbStrct->ninodes; i++, inodeTemp++) 
  {
    if ((inodeTemp == 0) || (inodeTemp->addrs == 0)) 
      continue;

    if (inodeTemp->type == 0) 
      continue;

    if (inodeTemp->type == 1 || inodeTemp->type == 2 || inodeTemp->type == 3) 
    { 
      
      // Iterate through direct blocks
      for (j = 0; j < NDIRECT; j++) 
      {
        if (inodeTemp->addrs[j] == 0) 
          continue;

        // Check if the value is within the range
        if (inodeTemp->addrs[j] <= finalBitBlock || inodeTemp->addrs[j] >= sbStrct->nblocks) 
          continue;

        uint isPresentBitmap = *(bitmapBlock+ (inodeTemp->addrs[j])/8) & Mask[(inodeTemp->addrs[j])%8];
        
        // If Bitmap is set to 0 : Error
        if (!isPresentBitmap) {
          fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
          exit(1);
        }
      }

      // check indirect blocks
      uint curIndirectAddr = inodeTemp->addrs[NDIRECT];
      if (curIndirectAddr == 0) 
        continue;

      else 
      {
        uint *indTemp = (uint *) (mmapbuf + curIndirectAddr * BLOCK_SIZE);
        for (i = 0; i < NINDIRECT; i++, indTemp++) 
        {      
          if ((indTemp == 0) || ((*indTemp) == 0)) 
            continue;
        
          uint isPresentBitmap = *(bitmapBlock+ (*indTemp)/8) & Mask[(*indTemp)%8];
          
          // If Bitmap is set to 0 : Error
          if (!isPresentBitmap) 
          {
            fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
            exit(1);
          }
        }
      }
    }
  }
}

void rule_6_check(int *flagUsed) {
  int i, j;
  char Mask[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

  struct dinode *inodeTemp = inodeStrct;

  uint inodeBlockCount = (sbStrct->ninodes/IPB) + 1;
  
  uint firstDataBlock = BBLOCK(IBLOCK(sbStrct->ninodes - 1), sbStrct->ninodes) + 1;
  char* bitmapBlock = (char*) ((char *)inodeTemp + inodeBlockCount * BLOCK_SIZE);
  
  // iterate through all inodes, mark used datablocks in isBlockUsed
  for (i = 0; i < sbStrct->ninodes; i++, inodeTemp++) 
  {
    if (inodeTemp->type == 0) 
    
      continue;
    if (inodeTemp->type == 1 || inodeTemp->type == 2 || inodeTemp->type == 3) 
    { 
      // check direct blocks
      for (j = 0; j < NDIRECT; j++) 
      {
        if (inodeTemp->addrs[j] == 0) 
          continue;
        // mark data block as used
        flagUsed[inodeTemp->addrs[j] - firstDataBlock] = 1;
      }

      // check indirect blocks
      uint indirectBlockAddr = inodeTemp->addrs[NDIRECT];
      if (indirectBlockAddr == 0) 
        continue;
      
      // Marking used block
      flagUsed[indirectBlockAddr - firstDataBlock] = 1;

      uint *curIndirectAddr = (uint *) (mmapbuf + indirectBlockAddr * BLOCK_SIZE);
      for (j = 0; j < NINDIRECT; j++, curIndirectAddr++) {
        if ((*curIndirectAddr) == 0) 
          continue;

        // marking used block
        flagUsed[(*curIndirectAddr) - firstDataBlock] = 1;
      }
    }
  }

  // iterate through datablocks
  for (i = 0; i < sbStrct->nblocks; i++) 
  {
    uint isPresentBitmap = *(bitmapBlock+ (firstDataBlock + i)/8) & Mask[(firstDataBlock + i)%8];
    if (flagUsed[i] == 0 && isPresentBitmap) 
    {
      fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
      exit(1);
    }
  }
}

//Auxiallary function for rule  7 and 8
void aux_rule_7_8(struct dinode *curInode, uint *cDirect, uint *cIndirect)
{
    int i;
    
    //Check for the direct block address
    for(i = 0;i<NDIRECT;i++)
    {
        //block is not used 
        if(curInode->addrs[i] == 0)
            continue;

        //counting address block use for direct
        cDirect[(curInode->addrs[i]) - (inodeBlockCount + bitmapBlockCount + 2)]++;

    }

    //Check for the indirect block address

    //check for valid indirect block start
    uint indirectBlockAddr = curInode->addrs[NDIRECT];
    
    //block not allocated :no point in further checks
    if(indirectBlockAddr == 0)
        return;
    
    //indirect block start address
    uint *curIndirectAddr = (uint *) (mmapbuf + indirectBlockAddr * BLOCK_SIZE);


    for(i = 0;i<NINDIRECT;i++,curIndirectAddr++)
    {
        //block is not used
        //value of the pointer holds the address 
        if((*curIndirectAddr)  == 0)
            continue;

        //counting address block use for direct // for Rule 8
        cIndirect[*(curIndirectAddr) - (inodeBlockCount + bitmapBlockCount + 2)]++;
        
    }

}



//Function to check for rule 7,8
void rule_7_8_check(uint *cDirect, uint *cIndirect)
{
    int i;
    
    //Looping through each inode
    struct dinode *inodetemp = inodeStrct;
    for(i = 0;i<sbStrct->ninodes;i++, inodetemp++)
    {
        //inode not allocated
        if(inodetemp->type == 0)
            continue;
        aux_rule_7_8(inodetemp, cDirect, cIndirect); 
        
    }

    //loop through the data blocks
    //Rule 6 , 7 and 8
    for(i=0;i<sbStrct->nblocks;i++)
    {
    
        // Check for Rule 7
        if(cDirect[i]>1)
        {
            fprintf(stderr, "ERROR: direct address used more than once.\n");
            exit(1);
        }

        // Check for Rule 8
        if(cIndirect[i]>1)
        {
            fprintf(stderr, "ERROR: indirect address used more than once.\n");
            exit(1);
        }

    }

}



void inodeRefCounter(uint *cInodeRef)
{
  int i, k, j;
  struct dirent *deTemp;
  struct dinode *inodetemp = inodeStrct;
  
  // Loop through the inodes
  for (i = 0; i < sbStrct->ninodes; i++, inodetemp++) 
  {
    
    //not allocated
    if (inodetemp->addrs == 0) 
      continue;
    
    //not in use
    if (inodetemp->type == 0) 
      continue;

    if (inodetemp->type == T_DIR) 
    { 
      if (inodetemp->addrs != 0) 
      {
        // check direct blocks
        for (j = 0; j < NDIRECT; j++) 
        {
          if (inodetemp->addrs[j] == 0) 
            continue;
          
          deTemp = (struct dirent *) (mmapbuf + (inodetemp->addrs[j])*BLOCK_SIZE);
          if (deTemp != 0) {
            
            // Loop through directory elements
            for (k = 0; k < DE_NUM; k++,deTemp++) 
            {
              if (deTemp == 0 || deTemp->inum == 0 || (strcmp(deTemp->name, ".")==0) || (strcmp(deTemp->name, "..")==0))
                continue;
              cInodeRef[deTemp->inum]++; 
            }
          }
        }
        
        // Indirect block check
        uint indirectBlockAddr = inodetemp->addrs[NDIRECT];
        
        if (indirectBlockAddr == 0)
          continue;
        
        uint *curIndirectAddr = (uint *) (mmapbuf + indirectBlockAddr * BLOCK_SIZE);
        for (j = 0; j < NINDIRECT; j++, curIndirectAddr++) 
        {
          
          if ((*curIndirectAddr) == 0) 
            continue;
          
          deTemp = (struct dirent *) (mmapbuf + (*curIndirectAddr)*BLOCK_SIZE);
          
          if (deTemp != 0) 
          {
            //Loop through directory elements
            for (k = 0; k < DE_NUM; k++,deTemp++) 
            {
              
              if (deTemp == 0 || deTemp->inum == 0 || (strcmp(deTemp->name, ".")==0) || (strcmp(deTemp->name, "..")==0))
                continue;
              
              cInodeRef[deTemp->inum]++; 
            }
          }
        }
      }
    }
  }
}


void rule_9_check(uint *cInodeRef)
{
    //Count references for each inode
    inodeRefCounter(cInodeRef);

    // int i;
    // for(i=0;i<sbStrct->ninodes;i++)
    //   printf("%d %d\n",i, cInodeRef[i]);

    struct dinode *inodetemp = inodeStrct;

    //loop through the inodes to check the count of references
    int i=2;
    inodetemp++;
    inodetemp++;
    while(i<sbStrct->ninodes)
    {
        //if node is used
        if(inodetemp->type != 0)
        {
            if(cInodeRef[i]==0)
            {
                
                fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
                exit(1);
            }
        }
        i++;
        inodetemp++;
    }
}

void rule_10_check(uint *cInodeRef)
{
    struct dinode *inodetemp = inodeStrct;

    //loop through the inodes to check the count of references
    int i=2;
    inodetemp++;
    inodetemp++;
    while(i<sbStrct->ninodes)
    {
        //if node is used
        if(cInodeRef[i]!=0)
        {
            if(inodetemp->type ==0)
            {
                fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
                exit(1);
            }
        }
        i++;
        inodetemp++;
    }
}

void rule_11_check(uint *cInodeRef)
{
    struct dinode *inodetemp = inodeStrct;

    //loop through the inodes to check the count of references
    int i=2;
    inodetemp++;
    inodetemp++;
    while(i<sbStrct->ninodes)
    {
        //if node is used
        if(inodetemp->type == T_FILE)
        {
            if(inodetemp->nlink != cInodeRef[i])
            {
                fprintf(stderr, "ERROR: bad reference count for file.\n");
                exit(1);
            }
            
        }
        i++;
        inodetemp++;
    }
}

void rule_12_check(uint *cInodeRef)
{
    struct dinode *inodetemp = inodeStrct;

    //loop through the inodes to check the count of references
    int i=2;
    inodetemp++;
    inodetemp++;
    while(i<sbStrct->ninodes)
    {
        //if node is used
        if(inodetemp->type == T_DIR)
        {
            if(cInodeRef[i] > 1)
            {
                fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
                exit(1);
            }
        }
        i++;
        inodetemp++;
    }
}


int
main(int argc, char *argv[])
{
    int fsfd, i;

    struct stat filebuf;
    int fstatus;

    //Error : When no image is provided
    if(argc < 2)
    {
        fprintf(stderr, "Usage: fcheck <file_system_image>\n");
        exit(1);
    }
    //Opening the file system given as output
    fsfd = open(argv[1], O_RDONLY);
    
    //Error : If the file system does'nt exist
    if(fsfd < 0)
    {
        fprintf(stderr, "image not found.\n");
        exit(1);
    }

    //fstat to get the size of the file system inputed
    //Exit : If file status is not properly retrived
    fstatus = fstat(fsfd, &filebuf);
    if(fstatus < 0)
    {
        exit(1);
    }

    mmapbuf = mmap(NULL, filebuf.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);

    //acessing super block start address
    sbStrct = (struct superblock *) (mmapbuf + 1 * BLOCK_SIZE);

    //acessing inode block start address
    inodeStrct = (struct dinode *) (mmapbuf + IBLOCK((uint)0)*BLOCK_SIZE);

    //storing number of inode blocks
    //IPB -> holds inodes per block
    //ninodes -> number of inodes
    inodeBlockCount = sbStrct->ninodes/IPB + 1;

    //acessing bitmap block start address
    //bitmap blocks located after inode blocks
    bitmapAddr = (char *) ((char*)inodeStrct + inodeBlockCount * BLOCK_SIZE);

    //storing number of bitmap blocks
    //BPB -> bitmap bits per block
    //size -> size of file system/number of blocks
    bitmapBlockCount = sbStrct->size/BPB + 1;

    //acessing data block start address
    //data blocks located after bitmap blocks
    dataAddr = (char *) (bitmapAddr + bitmapBlockCount * BLOCK_SIZE);

    //accessing root block start address
    directEntryStruct = (struct dirent *) (mmapbuf + (inodeStrct[ROOTINO].addrs[0])*BLOCK_SIZE);

    //block use/not in use flag
    int flagUsed[sbStrct->nblocks];
    
    //Direct address use count flag
    uint countDirect[sbStrct->nblocks];

    //Indirect address use count flag
    uint countIndirect[sbStrct->nblocks];

    //Inode reference counter
    uint countInodeRef[sbStrct->ninodes];

    //initializing 
    for(i=0;i<sbStrct->nblocks;i++)
        flagUsed[i] = 0;
    
    //initializing 
    for(i=0;i<sbStrct->nblocks;i++)
        countDirect[i] = 0;
    
    //initializing 
    for(i=0;i<sbStrct->nblocks;i++)
        countIndirect[i] = 0;

    //initializing 
    for(i=0;i<sbStrct->ninodes;i++)
        countInodeRef[i] = 0;


    //accessing data block

    //call function to check for rule 1
    rule_1_check();
    rule_2_check();
    rule_3_check();
    rule_4_check();
    rule_5_check();
    rule_6_check(flagUsed);
    rule_7_8_check(countDirect, countIndirect);
    rule_9_check(countInodeRef);
    rule_10_check(countInodeRef);
    rule_11_check(countInodeRef);
    rule_12_check(countInodeRef);


    return 0;

}