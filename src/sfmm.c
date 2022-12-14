/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h> //for ERNO


/*
 * This is your implementation of sf_malloc. It acquires uninitialized memory that
 * is aligned and padded properly for the underlying system.
 *
 * @param size The number of bytes requested to be allocated.
 *
 * @return If size is 0, then NULL is returned without setting sf_errno.
 * If size is nonzero, then if the allocation is successful a pointer to a valid region of
 * memory of the requested size is returned.  If the allocation is not successful, then
 * NULL is returned and sf_errno is set to ENOMEM.
 */

//sf_block* splithere(size, ptr){ //passing in the size of the malloc and pointer to that freeblock and 
 //you would split the freeblock into two blocks
 //then you make a footer and header where you should split it
 //you are setting the next block's previous footer to the freeblocks header
 //you have to move the freeblock down(left) 


static sf_block* epilogue;
static sf_block* prologue;

//prototype
void removefromfreelists(); //taking the block that you are allocating
void coelesing();
sf_block* splithere(); //passing in the size of the malloc and pointer to that freeblock
void setinitheads();
sf_block* findfreeblock(); //go through the linkedlist and find the first freeblock that has enough space to hold the allocated 
sf_header makeHeader();
void insertfreelist();
void removefromfreelists(); //taking the block that you are allocating
void initializeheap();
sf_size_t getsindexofquicklist();
void inserttoquicklistelsefreelist();

void coelesing(sf_block* freeblock){
    
    int prevallocated = ((freeblock->header)^MAGIC) & PREV_BLOCK_ALLOCATED; //gives 0 or 1 for prevallocated

    //Now to get the next
    void *blckptr = (void*)freeblock;

    int blksz = (freeblock->header ^ MAGIC) & 0xfffffff0;

    sf_block* nextblock = (sf_block*)(blckptr + (blksz));

    int nextalloc =((nextblock->header) ^ MAGIC) & THIS_BLOCK_ALLOCATED; //flag for 0 or 1 for nextallocated

    if (!prevallocated && nextalloc){
        //do merge with prev only 
         
       sf_header prevfooter = freeblock->prev_footer; //sf_footer previous footer
       uint64_t sizeprev = (prevfooter)^MAGIC;

       int prevprevalloc = sizeprev & PREV_BLOCK_ALLOCATED;

       sizeprev = sizeprev & 0xfffffff0;

       void* prevblockaddress = freeblock;
       prevblockaddress -= sizeprev;
       sf_block* prevblock = prevblockaddress;
       removefromfreelists(freeblock); //removing this
       removefromfreelists(prevblock); //removing this
       uint64_t freeblocksize = ((freeblock->header)^MAGIC) & 0xfffffff0;
       uint64_t totalheaders = freeblocksize+sizeprev;
       prevblock->header = (makeHeader(0, totalheaders, 0, prevprevalloc, 0))^MAGIC;

       void* footeraddress = prevblock;
       footeraddress += totalheaders;
       sf_footer* footerptr = footeraddress;
       *footerptr = prevblock->header;

       insertfreelist(prevblock); //add the total one 

    }

    else if(prevallocated && !nextalloc){
        
        //do merge with next only
    
       uint64_t freeblocksize = ((freeblock->header)^MAGIC) & 0xfffffff0;
       uint64_t nextblocksize = ((nextblock->header)^MAGIC) & 0xfffffff0;
       uint64_t totalheaders = freeblocksize + nextblocksize;
               
       removefromfreelists(freeblock); //removing this
       removefromfreelists(nextblock); //removing this

       freeblock->header = (makeHeader(0, totalheaders, 0, 1, 0))^MAGIC;

       void* footeraddress = freeblock;
       footeraddress += totalheaders;
       sf_footer* footerptr = footeraddress;
       *footerptr = freeblock->header;

        insertfreelist(freeblock); //add the total one 

    }

    else if (!prevallocated && !nextalloc){
        //do merge with prev and next

       sf_header* prevfooter = (void*)freeblock; //sf_footer previous footer
       uint64_t sizeprev = (*prevfooter)^MAGIC;

       int prevprevalloc = sizeprev & PREV_BLOCK_ALLOCATED;

       sizeprev = sizeprev & 0xfffffff0;

       void* prevblockaddress = freeblock;
       prevblockaddress -= sizeprev;
       sf_block* prevblock = prevblockaddress;
        
       removefromfreelists(prevblock); //removing this
       removefromfreelists(freeblock); //removing this

       uint64_t freeblocksize = ((freeblock->header)^MAGIC) & 0xfffffff0;
       uint64_t nextblocksize = ((nextblock->header)^MAGIC) & 0xfffffff0;
       uint64_t totalheaders = freeblocksize + nextblocksize +sizeprev;
       prevblock->header = (makeHeader(0, totalheaders, 0, prevprevalloc, 0))^MAGIC;

       void* footeraddress = prevblock;
       footeraddress += totalheaders;
       sf_footer* footerptr = footeraddress;
       *footerptr = prevblock->header;

       insertfreelist(prevblock); //add the total one 

       //now we gotta remove each of the free sublists from
       //and then add the combined to the free sublist

    }

    else{
        return;
    }
        //do nothing

}

sf_block* splithere(sf_size_t size,sf_block* ptr){ //passing in the size of the malloc and pointer to that freeblock

    //original header
    //ptr where free list starts
    removefromfreelists(ptr); //first remove
    sf_header orighdr = (ptr->header)^MAGIC;
    sf_header temp = orighdr;
    temp = temp & PREV_BLOCK_ALLOCATED;
    orighdr = orighdr & 0xfffffff0;

    sf_size_t paddedsize = size;
    paddedsize = paddedsize + 8; //add the header
    if (paddedsize < 32){
            paddedsize = 32; //set this
    }
    int remainder = 0;

    if (paddedsize >= 32){
        if (paddedsize % 16 != 0){
            remainder = paddedsize % 16;
        paddedsize = (16 - remainder) + paddedsize;
      }
    }

    sf_block* allocatedblck = ptr;
    allocatedblck->header = makeHeader(size, paddedsize, 1, temp, 0)^MAGIC;
    void* temp2 = ptr;
    temp2 = paddedsize + (void*)allocatedblck;
    ptr = temp2; //check if this is stored globally
    

    ptr->header = makeHeader(0,orighdr - paddedsize, 0, 1, 0)^MAGIC;
    void * footer = ptr;
    footer+=orighdr - paddedsize;
    sf_header *actualfooter =footer;
    *actualfooter = ptr->header;

    insertfreelist(ptr); //then return
    return allocatedblck;

}

void setinitheads(){

    for (int i = 0; i < NUM_FREE_LISTS; i++){
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i]; //& b/c it wants it set to the pointer
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i]; //& b/c it wants it set to the pointer
    }
}

sf_block* findfreeblock(sf_size_t size){ //go through the linkedlist and find the first freeblock that has enough space to hold the allocated 
    //#define NUM_FREE_LISTS 10
    //struct sf_block sf_free_list_heads[NUM_FREE_LISTS];

    for(int i=0; i < NUM_FREE_LISTS; i++){
        sf_block *temp = &sf_free_list_heads[i];
        sf_block* current = sf_free_list_heads[i].body.links.next; //used to iterate thru
        while(current != temp){
            uint64_t freelistsize = (current->header)^MAGIC; //gets the size of freelist
            freelistsize = freelistsize & 0xfffffff0;
            if (size < freelistsize){
                return current; // which is the block pointer we would need
            }
            if (size == freelistsize){
                removefromfreelists(current);
                return current; // which is the block pointer we would need
            }
            current = current->body.links.next;
        }
    }
    return NULL;
}

sf_header makeHeader(sf_size_t payloadsize, sf_size_t block, int allocated, int prevallocated, int qcklst){
    uint64_t header = 0; //makes empty header
    uint64_t payload = payloadsize;
    uint64_t blocksize = block; //right 32 bits

    payload = payload << 32; //shifting 32 bits to left because leftmost you wanna start in that portion
    header += payload;
    header += blocksize;

    if (allocated){
        header = header | THIS_BLOCK_ALLOCATED;
    } 

    if (prevallocated){
        header = header | PREV_BLOCK_ALLOCATED;
    }
    
    if(qcklst){
        header = header | IN_QUICK_LIST;
    }
    return header;
}

void insertfreelist(sf_block* freeblock){
    //Last in Last out
    //size of Freeblock
    uint64_t freeblocksize = (freeblock->header)^MAGIC;
    freeblocksize = freeblocksize & 0xfffffff0;

    
    if (freeblocksize == 32){
        freeblock ->body.links.prev = &sf_free_list_heads[0];
        freeblock ->body.links.next = sf_free_list_heads[0].body.links.next;

        sf_free_list_heads[0].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 64){
        freeblock ->body.links.prev = &sf_free_list_heads[1];
        freeblock ->body.links.next = sf_free_list_heads[1].body.links.next;

        sf_free_list_heads[1].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }    

        if (freeblocksize <= 128){
        freeblock ->body.links.prev = &sf_free_list_heads[2];
        freeblock ->body.links.next = sf_free_list_heads[2].body.links.next;

        sf_free_list_heads[2].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 256){
        freeblock ->body.links.prev = &sf_free_list_heads[3];
        freeblock ->body.links.next = sf_free_list_heads[3].body.links.next;

        sf_free_list_heads[3].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 512){
        freeblock ->body.links.prev = &sf_free_list_heads[4];
        freeblock ->body.links.next = sf_free_list_heads[4].body.links.next;

        sf_free_list_heads[4].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 1024){
        freeblock ->body.links.prev = &sf_free_list_heads[5];
        freeblock ->body.links.next = sf_free_list_heads[5].body.links.next;

        sf_free_list_heads[5].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 2048){
        freeblock ->body.links.prev = &sf_free_list_heads[6];
        freeblock ->body.links.next = sf_free_list_heads[6].body.links.next;

        sf_free_list_heads[6].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

        if (freeblocksize <= 4096){
        freeblock ->body.links.prev = &sf_free_list_heads[7];
        freeblock ->body.links.next = sf_free_list_heads[7].body.links.next;

        sf_free_list_heads[7].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize <= 8192){
        freeblock ->body.links.prev = &sf_free_list_heads[8];
        freeblock ->body.links.next = sf_free_list_heads[8].body.links.next;

        sf_free_list_heads[8].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }

    if (freeblocksize > 8192){
        freeblock ->body.links.prev = &sf_free_list_heads[9];
        freeblock ->body.links.next = sf_free_list_heads[9].body.links.next;

        sf_free_list_heads[9].body.links.next = freeblock;
        freeblock ->body.links.next ->body.links.prev = freeblock;
        return;
    }
    return;
}

void removefromfreelists(sf_block* block){ //taking the block that you are allocating
    block->body.links.prev->body.links.next = block->body.links.next; //first connection 
    block->body.links.next->body.links.prev = block->body.links.prev; //second connection
    block->body.links.next = NULL;
    block->body.links.prev = NULL;
    return;
}

void initializeheap(){
    //sf_set_magic(0x0); //set

    sf_mem_grow(); //grows by 1024 bytes
    prologue = sf_mem_start();
    prologue->header = makeHeader(0, 32, 1, 0, 0)^MAGIC; //changed this
    void *freeblk = prologue;
    freeblk+=32;
    sf_block *actual_freeblock = freeblk;

    actual_freeblock->header = makeHeader(0, (PAGE_SZ - 48), 0, 1, 0)^MAGIC; //calculates the size of freeblock
    
    actual_freeblock->body.links.next = actual_freeblock; //Makes it easier for inserting
    actual_freeblock->body.links.prev = actual_freeblock;
    insertfreelist(actual_freeblock);


    epilogue = sf_mem_end()-16;

    epilogue->prev_footer = actual_freeblock->header;
    epilogue->header = (makeHeader(0, 0, 1, 0, 0))^MAGIC;
}

void *sf_malloc(sf_size_t size) {
    //Find the padding size and estimate to nearest 16 , if its under 32 then you set to 32 the size
    //blocksize is atleast the size + 8 bytes (header) 

    if (size == 0) {
        return NULL;
    }
    if (size < 0){
        return NULL;
    }

    if(sf_mem_start() == sf_mem_end()){
        setinitheads();
        initializeheap(); //initializes the heap and works with freelist array

    }

    sf_size_t paddedsize = size;
    paddedsize = paddedsize + 8; //add the header
    if (paddedsize < 32){
            paddedsize = 32; //set this
    }
    int remainder = 0;

    if (paddedsize >= 32){
        if (paddedsize % 16 != 0){
            remainder = paddedsize % 16;
        paddedsize = (16 - remainder) + paddedsize;
      }
    }

    sf_block* ptr = findfreeblock(paddedsize); //finds the first block and returns a ptr to it
     //returns NULL to ptr if it can't find a freeblock
    if (ptr == NULL){
        void *memend = sf_mem_end()-16;

        if (sf_mem_grow() == NULL){
            sf_errno = ENOMEM;
            return NULL; //changed
        }
         else{
            int preval = 0;
            sf_block* newblockwithgrowedpage = memend;

            preval = 0;

            newblockwithgrowedpage->header = makeHeader(0, 1024, 0, preval, 0)^MAGIC; //CHECK IF LAST BLOCK IS ALLOCATED AND SET PREVALLOCATED

            void *footer = newblockwithgrowedpage;
            footer+=1024;
            sf_footer *actualfooter = footer;
            *actualfooter = newblockwithgrowedpage->header;
            actualfooter+=1;
            *actualfooter = makeHeader(0, 0, 1, 0, 0)^ MAGIC;
            insertfreelist(newblockwithgrowedpage);
            coelesing(newblockwithgrowedpage); //you would do colesing here 

            return sf_malloc(size);

            //Coelesing
            //Grow the page 1024 bytes
            //If there were leftovers, you would carry it over to the next page
            //Check the ending of first page block and the starting of 2nd page block and you would have merge if needed
            
        }

    }
    uint64_t blocksize = ptr->header;
    blocksize = blocksize ^ MAGIC;
    int isprevalloc = blocksize & PREV_BLOCK_ALLOCATED;
    blocksize = blocksize & 0xfffffff0;

    if(blocksize == paddedsize){
        ptr->header = makeHeader(size, paddedsize, 1, isprevalloc, 0)^MAGIC;
        return ptr->body.payload;
    }
    sf_block *p = splithere(size, ptr); //passing in the size of the malloc and pointer to that freeblock
    return p->body.payload;
    // TO BE IMPLEMENTED
}   

/*
 * Marks a dynamically allocated region as no longer in use.
 * Adds the newly freed block to the free list.
 *
 * @param ptr Address of memory returned by the function sf_malloc.
 *
 * If ptr is invalid, the function calls abort() to exit the program.
 */
sf_size_t getsindexofquicklist(sf_size_t size) {
    if ((size % 16) != 0) {
        return 15;
    }
                //minimum size is 32 //16 is the aligner
    return (size - 32)/16; //or you would get the index
}

void inserttoquicklistelsefreelist(sf_block* freeblock){
    //insert to quick list if its index is from 0 - 9

    uint64_t freeblocksize = (freeblock->header)^MAGIC;
    freeblocksize = freeblocksize & 0xfffffff0;

    int index = getsindexofquicklist(freeblocksize); // you would get index

    if (index > 9){
        sf_block *newblock = (void*)freeblock + freeblocksize; //makes the new block there
        newblock->header = (((newblock->header)^MAGIC) & 0xFFFFFFFFFFFFFFFD)^MAGIC; //sets the prevblock of newblock to 0
        freeblock->header = (((freeblock->header)^MAGIC) & 0xFFFFFFFFFFFFFFFB)^MAGIC; //sets the alloc to 0 of freeblock
        newblock->prev_footer = freeblock->header;

        //after setting them to 0 you would add to freelist
        insertfreelist(freeblock);
        coelesing(freeblock);
        return;
    }
    else{
         freeblock->header = (((freeblock->header)^MAGIC) | THIS_BLOCK_ALLOCATED)^MAGIC;
        
          if (sf_quick_lists[index].length == 0){
            sf_quick_lists[index].first = freeblock; //if this, then you set the head
          }
          else{
              freeblock->body.links.next = sf_quick_lists[index].first; //set the new next
              sf_quick_lists[index].first = freeblock; //set the new head
          }
          
        }

    }

void sf_free(void *pp) {
    // TO BE IMPLEMENTED
    if (pp == NULL){ //condition
        abort();
    }
    if (!((uintptr_t)pp % 16 == 0))
        abort(); //conditon
    void *temp = pp;
    temp -= 16;
    sf_block* block = temp;
    sf_header header = (block->header)^MAGIC;
    uint64_t size = header & 0xFFFFFFF0;
    if ((header & 0xfffffff0) < 32){
        abort();
    } 

    if(!((header & 0xfffffff0) % 16 == 0)){
        abort();
    }
    void* prologuelocation = prologue; //location of prologue
    void* epiloguelocation = epilogue; //location of epilogue

    if(((uintptr_t)prologuelocation >= (uintptr_t)block) || ((uintptr_t)epiloguelocation <= (uintptr_t)block)){
        abort();
    }

    if((header & THIS_BLOCK_ALLOCATED) == 0){
        abort();
    }

    if((header & PREV_BLOCK_ALLOCATED) == 0){
      sf_size_t thesizeofprevblock = ((block->prev_footer)^MAGIC) & 0xfffffff0;
      void* move = (void*)block;
      move -= thesizeofprevblock;
      sf_block* prevblock = move;
      sf_header preblockhdr = (prevblock->header)^MAGIC;
      if((preblockhdr & THIS_BLOCK_ALLOCATED) != 0){
          abort();
      }
    }
    int prevalloc = (header & PREV_BLOCK_ALLOCATED);
    header = header & 0x00000000ffffffff; //removes last 32 bits , keeps first 32 bits when going right to left
    block->header = makeHeader(0, size, 0, prevalloc, 0);
    void* footer = block;
    footer +=  size;

   sf_footer *actualfooter= footer;
   *actualfooter = (header)^MAGIC;
   block->header = *actualfooter;
   inserttoquicklistelsefreelist(block);

}

/*
 * Resizes the memory pointed to by ptr to size bytes.
 *
 * @param ptr Address of the memory region to resize.
 * @param size The minimum size to resize the memory to.
 *
 * @return If successful, the pointer to a valid region of memory is
 * returned, else NULL is returned and sf_errno is set appropriately.
 *
 *   If sf_realloc is called with an invalid pointer sf_errno should be set to EINVAL.
 *   If there is no memory available sf_realloc should set sf_errno to ENOMEM.
 *
 * If sf_realloc is called with a valid pointer and a size of 0 it should free
 * the allocated block and return NULL without setting sf_errno.
 */

void *sf_realloc(void *pp, sf_size_t rsize) {
    // TO BE IMPLEMENTED
    if (pp == NULL){ //condition
        sf_errno = ENOMEM; //sets
        abort();
    }

    if (!((uintptr_t)pp % 16 == 0)){
        sf_errno = ENOMEM; //sets
        abort();
    }

    void *temp = pp;
    temp -= 16;
    sf_block* block = temp;
    sf_header header = (block->header)^MAGIC;
    int prevalc = header & PREV_BLOCK_ALLOCATED;
    uint64_t size = header & 0xFFFFFFF0; //this the block size you & to ignore the last 

    if ((header & 0xfffffff0) < 32){
        sf_errno = ENOMEM; //sets
        abort();

    } 

    if(!((header & 0xfffffff0) % 16 == 0)){
        sf_errno = ENOMEM; //sets
        abort();
    }
    void* prologuelocation = prologue; //location of prologue
    void* epiloguelocation = epilogue; //location of epilogue

    if(((uintptr_t)prologuelocation >= (uintptr_t)block) || ((uintptr_t)epiloguelocation <= (uintptr_t)block)){
        sf_errno = ENOMEM; //sets
        abort();
    }

    if((header & THIS_BLOCK_ALLOCATED) == 0){
        sf_errno = ENOMEM; //sets
        abort();
    }
    if((header & PREV_BLOCK_ALLOCATED) == 0){
      sf_size_t thesizeofprevblock = ((block->prev_footer)^MAGIC) & 0xfffffff0;
      void* move = (void*)block;
      move -= thesizeofprevblock;
      sf_block* prevblock = move;
      sf_header preblockhdr = (prevblock->header)^MAGIC;
      if((preblockhdr & THIS_BLOCK_ALLOCATED) != 0){
        sf_errno = ENOMEM; //sets
        abort();
      }
    }
    //until here the pointer is valid
     
     sf_size_t paddedsize = rsize;
        paddedsize = paddedsize + 8; //add the header
        if (paddedsize < 32){
            paddedsize = 32; //set this
        }
        int remainder = 0;

        if (paddedsize >= 32){
            if (paddedsize % 16 != 0){
                remainder = paddedsize % 16;
            paddedsize = (16 - remainder) + paddedsize;
            }
        }

    if(rsize == 0){ //size parameter is 0, free the block and return NULL.
        free(block);
        return NULL;
     }

    if(size < paddedsize){ //this is when reallocating to a larger size

        sf_block* theblockgivenback = sf_malloc(rsize);

        if (theblockgivenback == NULL){
            return NULL;
        }
        else{
            memcpy(pp, theblockgivenback->body.payload, (rsize)); //casted rsize

            sf_free(block->body.payload); //frees the block

            return theblockgivenback->body.payload;
         }
    }
    else{

        if (size - paddedsize < 32 && size > paddedsize){ //block k

            //FIRST SUBCASE
     
            //ptr where free list starts
            block->header = (makeHeader(rsize, size, 1, prevalc, 0))^MAGIC;
            return block->body.payload;
  
        /////////////
         }

        else{
            //SECOND SUBCASE
            sf_block* allocatedblock = splithere(rsize,block); //blocksize and ptr to block

          //  header = (((block->header ^ MAGIC) & 0x00000000FFFFFFFF) | (rsize << 32))^MAGIC;
            allocatedblock->header = (makeHeader(rsize, paddedsize, 1, prevalc, 0))^MAGIC;

            void* nextblock = allocatedblock;
            nextblock += paddedsize;
            sf_block* nextblockhere = nextblock;

            coelesing(nextblockhere);

            return allocatedblock->body.payload;
        }
    }

}
 
double sf_internal_fragmentation() {
    // TO BE IMPLEMENTED
    int payload = 0;
    int allocatedblocksize = 0;

    //start at the beginning
    sf_block* currentblock = (void*)prologue + 32;
    //0x00000000ffffffff

    while (currentblock != epilogue){     
        uint64_t current = (currentblock->header)^MAGIC;
        int maskedvar = current & 0xfffffff0;
        long payloadfirst = (current & 0xffffffff00000000) >> 32;

        if((current & THIS_BLOCK_ALLOCATED) && (!(current & IN_QUICK_LIST))){
             payload += payloadfirst;
             allocatedblocksize += maskedvar;
        }
        
        void* nextblock = currentblock;
        nextblock += maskedvar;
        currentblock = nextblock;
  
    }
        if(allocatedblocksize == 0){
            return 0.0;
        }

    return ((double)payload / (double)allocatedblocksize);
    
}

double sf_peak_utilization() {
    // TO BE IMPLEMENTED
    return 0.0; //miips cheat code
}
