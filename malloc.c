/*  
 * mm.c 
 *  Our design is to maintain a binary search tree to manage the free blocks with different sizes. 
 *	We use a free block itself to be the tree node.  
 *  Every node in tree is also the head of a doubly linked list where all node are of same size. 
 *  Node in free_tree has four attributes left-child, right-child, parent, and brothers. 
 *  And every attribute is use a word size space in free block to perform.
 
 *	And if the node is in linked list which is node a tree node, 
 *	we use left-child to save the address of previous free block in this list, 
 *	and use brother to save the address of next free block in this list,
 *	also assign the right-child to -1 which differ from the tree node which do not have a right-child(on this case, assign it to 0).
 *
 *	Besides, it is wrapped by a 4 bytes header and a 4 bytes footer, which are used for coalescing
 *	
 *	The minimum size of a free block is 24 bytes.  
 *	When the size we need to allocate is no bigger than 512, we allocate a space with size of upper bond in (2^n)+4+4. 
 *	For example, when we need 10 bytes space, the upper bound in (2^n)+4+4 is (2^4)+4+4=24. 
 *	When the size we need is bigger than 512, we allocate a space with the size we need and other 8 bytes for pointers.
 *
 *  We control the check program by two variables: showflag and checkflag. 
 *	Only when these two are set to 1, the check few program can operate. 
 *	So we don't need to put them into comments.
 *
 *	Team member:
 *	Xiangtai Hou, netID: xhb083
 *	Haomin Zeng, netID: hzy075
 * 
 *
 *  Free BLOCK
 *        bp
 *        |
 *  ======================================================
 * | HEAD | LEFT | RIGHT | PART | BROS |            | FOOT |
 *  ======================================================
 *         (prev)  (-1)          (next)          ---------- when it is in the doubly linked list headed by tree node
 *
 *  ALLOCATED BLOCK 
 *        bp
 *        |
 *  ======================================================
 * | HEAD |   PAYLOAD                                | FOOT |
 *  ======================================================
 *
 */    
#include <stdio.h>    
#include <stdlib.h>    
#include <assert.h>    
#include <unistd.h>    
#include <string.h>    
    
#include "mm.h"    
#include "memlib.h"    
   
team_t team = {
    /* Team name */
    "xth_hmz",
    /* First member's full name */
    "Xiangtai Hou",
    /* First member's email address */
    "xiangtaihou2014@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Haomin Zeng",
    /* Second member's email address (leave blank if none) */
    "haominzeng2014@u.northwestern.edu"
};
 

/*
*  Constants and macros
*/
 
#define WSIZE 4   
#define DSIZE 8   
#define CHUNKSIZE (1<<10)//Page size in bytes
#define MINSIZE 24
  
// Define the alignment, single word(4) or double word(8) alignment
#define ALIGNMENT 8 

// Get the maximum number of two numbers
#define MAX(x,y) ((x)>(y)? (x): (y))  
  
// Read and write a value from the address p
#define GET(p)     (*(size_t *)(p))  
#define PUT(p,val) (*(size_t *)(p)=(val))  
  
// Get the size of the block from the header p of the block
#define GET_SIZE(p)  ((GET(p))&~0x7)  
#define GET_ALLOC(p) (GET(p)&0x1)  

// Pack a size and allocated bit into a word
#define PACK(size,alloc) ((size)|(alloc))  
  
// Get information from the pointer bp
// bp points to the the place right after the Head of the block
#define HEAD(bp) 	((void *)(bp)-WSIZE)  
#define LEFT(bp) 	((void *)(bp))  
#define RIGHT(bp)   ((void *)(bp)+WSIZE)  
#define PART(bp)    ((void *)(bp)+DSIZE)  
#define BROS(bp)    ((void *)(bp)+((WSIZE<<1)+WSIZE))  
#define FOOT(bp)    ((void *)(bp)+GET_SIZE(HEAD(bp))-DSIZE)  
//Get bp 
#define GET_HEAD(bp) (GET(HEAD(bp)))
#define GET_FOOT(bp) (GET(FOOT(bp)))
#define GET_PART(bp) (GET(PART(bp)))
#define GET_BROS(bp) (GET(BROS(bp)))
#define GET_LEFT_CHILD(bp)  (GET(LEFT(bp)))
#define GET_RIGHT_CHILD(bp) (GET(RIGHT(bp)))
//Set bp
#define PUT_HEAD(bp,val) (PUT(HEAD(bp),(int)val))
#define PUT_FOOT(bp,val) (PUT(FOOT(bp),(int)val))
#define PUT_PART(bp,val) (PUT(PART(bp),(int)val))  
#define PUT_BROS(bp,val) (PUT(BROS(bp),(int)val))
#define PUT_LEFT_CHILD(bp,val)  (PUT(LEFT(bp),(int)val))
#define PUT_RIGHT_CHILD(bp,val) (PUT(RIGHT(bp),(int)val))

// Rounds up to the nearest multiple of Alignment
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (((size) + (ALIGNMENT - 1)) & ~0x7) 
    
// Given block bp, compute address of next and previous blocks
#define PREV_BLKP(bp) ((void *)(bp)-GET_SIZE(((void *)(bp)-DSIZE)))  
#define NEXT_BLKP(bp) ((void *)(bp)+GET_SIZE(HEAD(bp)))  

  


int mm_init ();  
void *mm_malloc (size_t size);  
void mm_free (void *bp);  
void *mm_realloc (void *bp,size_t size);  

static void *coalesce (void *bp);
static void *extend_heap (size_t size);

static void *find_fit (size_t asize);
static void place (void *ptr,size_t asize);  

static void add_node (void *bp);
static void delete_node (void *bp); 

static void checkblock(void *bp);  
static void mm_check();  
  
static void *heap_listp = 0;  
static void *free_tree = 0;//free tree

static size_t flag = 0;  //control flag
static int showflag = 0;// When the showflag = 1, print the entry information
static int checkflag = 0;// When the checkflag = 1, active those check functions

 
  
/* 
 * mm_init - initialize the malloc package.
 * call function extend_heap
 */ 
int mm_init()  
{  
	
    free_tree = NULL;
    
    //checkpoint
    if(showflag == 1){
		printf("begin to initialize the heap\n");
	}
	
    if ((heap_listp = mem_sbrk((WSIZE<<2))) == (void*) -1){
        return -1;  
	}
	
	// Create the initial empty heap
    PUT(heap_listp,0);						
    PUT(heap_listp+(WSIZE),PACK(DSIZE,1)); //Prologue header//WSIZE*1
    PUT(heap_listp+(WSIZE<<1),PACK(DSIZE,1)); //Prologue footer//WSIZE*2
	PUT(heap_listp+((WSIZE<<1)+WSIZE),PACK(0,1));     //Epilogue//WSIZE*3
	
	
    heap_listp += (WSIZE<<2);  				//heap_listp points to bp of first valid block
     
    
	
    if (extend_heap(CHUNKSIZE) == NULL)  
        return -1;  
	
	//checkpoint
	if(showflag == 1){
		printf("initialize successfully");
	}
	
    return 0;  
}  

/*
 * extend_heap return a block whose size is an integral number of double words
 * insert the block to the free list
 * call the function coalesce and mem_sbrk, add_node
 */
void *extend_heap(size_t size)
{
    if(showflag){
		printf("begin to extend_heap\n");
	}
	void *bp = NULL;
    void *coalesced_bp = 0;
    
    if ((long)(bp=mem_sbrk(size)) == -1){
        if(showflag)
        {
			printf("extend heap unsuccessfully\n");
		}
		return NULL;
	}
    
	//printf("bp = %p,size = %d\n",bp,size);
	// Initialize free block header/footer and the epilogue header
    PUT_HEAD(bp,PACK(size,0));//free block header
    PUT_FOOT(bp,PACK(size,0));//free block footer
    PUT_HEAD(NEXT_BLKP(bp),PACK(0,1));//new epilogue header
	
	if(checkflag == 1){
		checkblock(bp);
	}
    
	// Coalesce if the previous block was free
    coalesced_bp = coalesce(bp);
    add_node(coalesced_bp);
	if(showflag){
		printf("extend heap successfully\n");
	}
	if(checkflag == 1){
		checkblock(coalesced_bp);
	}
	
    return bp;
}

/*
 * mm_free - Freeing a block does nothing, and add it to free_tree
 * call function add_node,
 */
void mm_free(void *bp)
{
	//checkpoint
    if(showflag ==1)
		printf("begin to free\n");
    
    size_t size = GET_SIZE(HEAD(bp));

    PUT(HEAD(bp),PACK(size,0));
    PUT(FOOT(bp),PACK(size,0));//!!!!
	
    add_node(coalesce(bp));
    //checkpoint
    if(showflag == 1)
		printf("free successfully and add it to free_tree successfully\n");
	
}


/*
 * coalesce function
 * we choose free list format with prologue and no epilogue blocks that are always marked as allocated
 * use the basic 4 situations to coalesce
 * call delete node
 */

static void *coalesce(void *bp)
{
	if(showflag ==1){
		printf("begin to coalesce\n");
	}
    size_t prev_alloc = GET_ALLOC(HEAD(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HEAD(NEXT_BLKP(bp)));
    size_t bsize = GET_SIZE(HEAD(bp));
    void *prev_block = PREV_BLKP(bp);
    void *next_block = NEXT_BLKP(bp);
    int ca=0;
	
    //checkpoint
	if(checkflag == 1){
		printf("check the prev_block\n");
		checkblock(prev_block);
		printf("check the current block\n");
		checkblock(bp);
		printf("check the next_block\n");
		checkblock(next_block);
	}
    
    if (prev_alloc && next_alloc)
        ca = 1;
    else if (prev_alloc && (!next_alloc))
        ca = 2;
    else if ((!prev_alloc) && next_alloc)
        ca = 3;
    else
        ca = 4;
    
    switch (ca)
    {
        case 1:// case 1 prev and next are allocated
            if(showflag == 1){
                printf("case1: coalesce successfully\n");
            }
            return bp;
        case 2://case 2 prev is allocated and next is free
            //checkpoint
            if(showflag == 1){
                printf("coalesce_case2\n");
            }
            bsize += GET_SIZE(HEAD(next_block));
            delete_node(next_block);
            PUT_HEAD(bp,PACK(bsize,0));
            PUT_FOOT(bp,PACK(bsize,0));
            //checkpoint
            if(showflag == 1){
                printf("case2: coalesce successfully\n");
            }
            //checkpoint
            if(checkflag == 1){
                checkblock(bp);
            }
            
            return bp;
        case 3://case 3 prev is free and next is allocated
            //checkpoint
            if(showflag == 1){
                printf("coalesce_case3\n");
            }
            bsize += GET_SIZE(HEAD(prev_block));
            delete_node(prev_block);
            PUT_HEAD(prev_block,PACK(bsize,0));
            PUT_FOOT(bp,PACK(bsize,0));
            
            //checkpoint
            if(showflag == 1){
                printf("case3: coalesce successfully\n");
            }
            //checkpoint
            if(checkflag == 1){
                checkblock(prev_block);
            }
            
            return prev_block;
        case 4://case 4 prev and next are both free
            //checkpoint
            if(showflag == 1){
                printf("coalesce_case4\n");
            }
            bsize += GET_SIZE(HEAD(prev_block))+GET_SIZE(HEAD(next_block));
            delete_node(next_block);
            delete_node(prev_block);
            PUT_HEAD(prev_block,PACK(bsize,0));
            PUT_FOOT(next_block,PACK(bsize,0));
            
            //checkpoint
            if(showflag == 1){
                printf("case4: coalesce successfully\n");
            }
            //checkpoint
            if(checkflag == 1){
                checkblock(prev_block);
            }
            
            
            return prev_block;
            
    }

    
}  


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 * Always allocate a block whose size is a multiple of the alignment.
 * Allocate the size after adjusted according to input size 
 * call function find_fit , place and extend_heap
 */
void *mm_malloc(size_t size)  
{
    //checkpoint
	if(showflag ==1){
		printf("begin to malloc\n");
	}
    size_t asize = 0;   
    size_t extendsize = 0;  
    void *bp = 0;
    
    
	//checkpoint
	if(checkflag == 1)
		mm_check();
	
    //ignore spurious requests
    if (size <= 0)
    {
        printf("Invalid request.");
        return NULL;
    }
	// Adjust the input size to a nearest larger number which is power of 2
    if (size <= MINSIZE-8)  
        asize = MINSIZE-8;  
    else if (size <= 32) 
		asize = 32;
	else if (size <= 64)
		asize = 64;
	else if (size <= 128)
		asize = 128;
	else if (size <= 256)
		asize = 256;
	else if (size <= 512)
		asize = 512;
    else     
	{	
        asize = ALIGN(size);  
    } 
	
	// Add the foot and head block
	asize = asize + 8;
    bp = find_fit(asize);  
  
    
    if (bp != NULL)  
    {  
        place(bp,asize);  
		//checkpoint
		if(showflag == 1){
			printf("find fitted block, malloc successfully\n");
		}
		if(checkflag == 1){
			checkblock(bp);
		}
		
    }  
    else  
    {  
        extendsize = MAX(asize , CHUNKSIZE); 
        if(bp = extend_heap(extendsize) == NULL){  
			return NULL;
		}
        if ((bp = find_fit(asize)) == NULL){  
            return NULL;  
		}
        place(bp,asize);  
		
        //checkpoint
		if(showflag == 1){
			printf(" after extend head, malloc successfully\n");
		}
		if(checkflag == 1){
			checkblock(bp);
		}
    }
    return bp;
    
    
}  


/*
 *  function find_fit
 *  use best fit search,use while to get it
 */
static void* find_fit(size_t asize)
{
    if(showflag == 1){
		printf("begin to find fit\n");
	}
	void *free_root = free_tree;
    void *free_fit = NULL;
    
    if (free_tree == NULL)
        return free_fit;

    for( ;free_root != NULL; )
    {
        if (asize <= GET_SIZE(HEAD(free_root)))
        {
            free_fit = free_root;
            free_root = GET_LEFT_CHILD(free_root);
        }
        else if(asize > GET_SIZE(HEAD(free_root)))
            free_root = GET_RIGHT_CHILD(free_root);
    }
    
    //checkpoint
	if(showflag == 1){
		printf("find fit successfully\n");
	}
	if(checkflag == 1){
		checkblock(free_fit);
	}
    
    return free_fit;
}  


/*
 * function place
 * get the address bp whose size of it is asize
 */
static void place(void *bp,size_t asize)
{
    //checkpoint
    if(showflag ==1){
		printf("begin to place\n");
	}
    void *coalesced_bp = NULL;
    
	size_t csize = GET_SIZE(HEAD(bp));
    
    delete_node(bp);

    if ((csize-asize)>=24)				//while the block can be divided into two illegal blocks
    {
        PUT_HEAD(bp,PACK(asize,1));
        PUT_FOOT(bp,PACK(asize,1));
        bp=NEXT_BLKP(bp);
        PUT_HEAD(bp,PACK(csize-asize,0));
        PUT_FOOT(bp,PACK(csize-asize,0));

		add_node(bp);
    }
    else
    {
        PUT_HEAD(bp,PACK(csize,1));
        PUT_FOOT(bp,PACK(csize,1));
    }
    
    
    
    //checkpoint
	if(showflag == 1){
		printf("place successfully\n");
	}
}


 
/*
* mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr,size_t size)  
{  
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}  
  

static void add_node(void *bp)
{
    int ca = 0;
    //checkpoint
    if(showflag == 1){
		printf("begin to add node\n");
	}
	if(checkflag == 1){
		mm_check();
		check(bp);
	}
    
    if (free_tree == 0)// if there is no free block in free-tree
    {
        free_tree = bp;
        PUT_LEFT_CHILD(bp,0);
        PUT_RIGHT_CHILD(bp,0);
        PUT_PART(bp,0);
        PUT_BROS(bp,0);
        return;
    }
    
    void *my_tr = free_tree;
    void *par_my_tr = 0;
	
	if(checkflag == 1){
		checkblock(free_tree);
	}
    
    while(1)
    {
        if (GET_SIZE(HEAD(bp)) > GET_SIZE(HEAD(my_tr)))
            if (GET_RIGHT_CHILD(my_tr) != 0)
                my_tr = GET_RIGHT_CHILD(my_tr);
            else break;
        else if (GET_SIZE(HEAD(bp)) < GET_SIZE(HEAD(my_tr)))
            if (GET_LEFT_CHILD(my_tr) != 0)
                my_tr = GET_LEFT_CHILD(my_tr);
            else break;
        else if (GET_SIZE(HEAD(bp)) == GET_SIZE(HEAD(my_tr)))
            break;
    }
	
    if ((GET_SIZE(HEAD(bp)) < GET_SIZE(HEAD(my_tr))))
        ca = 1;
    else if (GET_SIZE(HEAD(bp)) > GET_SIZE(HEAD(my_tr)))
        ca = 2;
    else if ((GET_SIZE(HEAD(bp)) == GET_SIZE(HEAD(my_tr)))&&(my_tr != free_tree))
        ca = 3;
    else if ((GET_SIZE(HEAD(bp)) == GET_SIZE(HEAD(my_tr)))&&(my_tr == free_tree))
        ca = 4;
    switch (ca) {
        case 1:
            PUT_LEFT_CHILD(my_tr,bp);
            PUT_PART(bp,my_tr);
            PUT_BROS(bp,0);
            PUT_LEFT_CHILD(bp,0);
            PUT_RIGHT_CHILD(bp,0);
            break;
        case 2:
            PUT_RIGHT_CHILD(my_tr,bp);
            PUT_PART(bp,my_tr);
            PUT_BROS(bp,0);
            PUT_LEFT_CHILD(bp,0);
            PUT_RIGHT_CHILD(bp,0);
            break;
        case 3:
            if (GET_SIZE(HEAD(GET_PART(my_tr))) >  GET_SIZE(HEAD(my_tr)))
                PUT_LEFT_CHILD(GET_PART(my_tr),bp);
            else
                PUT_RIGHT_CHILD(GET_PART(my_tr),bp);
            
            PUT_LEFT_CHILD(bp,GET_LEFT_CHILD(my_tr));
            PUT_RIGHT_CHILD(bp,GET_RIGHT_CHILD(my_tr));
            if (GET_LEFT_CHILD(my_tr) != 0)
                PUT_PART(GET_LEFT_CHILD(my_tr),bp);
            if (GET_RIGHT_CHILD(my_tr) != 0)
                PUT_PART(GET_RIGHT_CHILD(my_tr),bp);
            PUT_PART(bp,GET_PART(my_tr));
            PUT_BROS(bp,my_tr);
            PUT_RIGHT_CHILD(my_tr,-1);
            PUT_LEFT_CHILD(my_tr,bp);
            break;
        case 4://set to root
            free_tree = bp;
            PUT_LEFT_CHILD(bp,GET_LEFT_CHILD(my_tr));
            PUT_RIGHT_CHILD(bp,GET_RIGHT_CHILD(my_tr));
            if (GET_LEFT_CHILD(my_tr) != 0)
                PUT_PART(GET_LEFT_CHILD(my_tr),bp);
            if (GET_RIGHT_CHILD(my_tr) != 0)
                PUT_PART(GET_RIGHT_CHILD(my_tr),bp);
            PUT_PART(bp,0);
            PUT_BROS(bp,my_tr);
            
            PUT_LEFT_CHILD(my_tr,bp);
            PUT_RIGHT_CHILD(my_tr,-1);
            break;
            
    }
	return;
}

static void delete_node(void *bp)  
{  
    //checkpoint
    if(showflag ==1){
		printf("begin to delete!\n");
	}
	if(checkflag == 1){
		mm_check();
		checkblock(bp);
	}
	
	
	if (bp == free_tree)////////////////// bp is root////////////////////////////////////////////////////////////////////////////////////////
    {  
        if (GET_BROS(bp) != 0)    //if there are more than one free block in this level
        {  
            free_tree = GET_BROS(bp);  // set the second one to be the first one 
            PUT_LEFT_CHILD(free_tree,GET_LEFT_CHILD(bp));  
            PUT_RIGHT_CHILD(free_tree,GET_RIGHT_CHILD(bp));  
            if (GET_RIGHT_CHILD(bp) != 0)  
                PUT_PART(GET_RIGHT_CHILD(bp),free_tree);  
            if (GET_LEFT_CHILD(bp) != 0)  
                PUT_PART(GET_LEFT_CHILD(bp),free_tree);  
            return;  
        }  
        else// there is only one free block in this level
        {  
            if (GET_LEFT_CHILD(bp) == 0)        // no left child  
                free_tree = GET_RIGHT_CHILD(bp);  
            else if (GET_RIGHT_CHILD(bp) == 0)  // no right child   
                free_tree = GET_LEFT_CHILD(bp);  
            else                                // have two children
            {  
                void *my_tr = GET_RIGHT_CHILD(bp);  
                while (GET_LEFT_CHILD(my_tr) != 0)  
                    my_tr = GET_LEFT_CHILD(my_tr);  
					
                free_tree = my_tr; 
				
                if (GET_LEFT_CHILD(bp) != 0)  
                    PUT_PART(GET_LEFT_CHILD(bp),my_tr); 
					
                if (my_tr != GET_RIGHT_CHILD(bp))  
                {  
                    if (GET_RIGHT_CHILD(my_tr) != 0)  
                        PUT_PART(GET_RIGHT_CHILD(my_tr),GET_PART(my_tr));  
						
                    PUT_LEFT_CHILD(GET_PART(my_tr),GET_RIGHT_CHILD(my_tr));  
                    PUT_RIGHT_CHILD(my_tr,GET_RIGHT_CHILD(bp));  
                    PUT_PART(GET_RIGHT_CHILD(bp),my_tr);  
                }  
                PUT_LEFT_CHILD(my_tr,GET_LEFT_CHILD(bp));  
            }  
        }  
    }  
    else//////////////////////////////////if bp is not root///////////////////////////////////////////////////////////////////////////
    {  
        if (GET_RIGHT_CHILD(bp) == -1)///////////////////////////////////////////// bp is not the first one in this level
        {// not the first block in the node
            if (GET_BROS(bp) != 0)
                PUT_LEFT_CHILD(GET_BROS(bp),GET_LEFT_CHILD(bp));
            PUT_BROS(GET_LEFT_CHILD(bp),GET_BROS(bp));
        }
        else if (GET_RIGHT_CHILD(bp) != -1 && GET_BROS(bp) != 0)//////// the first one but not the only one in this level
        {// the first block in the node
            
            if (GET_SIZE(HEAD(bp)) > GET_SIZE(HEAD(GET_PART(bp))))
                PUT_RIGHT_CHILD(GET_PART(bp),GET_BROS(bp));
            else
                PUT_LEFT_CHILD(GET_PART(bp),GET_BROS(bp));
            
            PUT_LEFT_CHILD(GET_BROS(bp),GET_LEFT_CHILD(bp));
            PUT_RIGHT_CHILD(GET_BROS(bp),GET_RIGHT_CHILD(bp));
			//
            if (GET_LEFT_CHILD(bp) != 0)
                PUT_PART(GET_LEFT_CHILD(bp),GET_BROS(bp));
            if (GET_RIGHT_CHILD(bp) != 0)
                PUT_PART(GET_RIGHT_CHILD(bp),GET_BROS(bp));
            PUT_PART(GET_BROS(bp),GET_PART(bp));
        }
        else if (GET_RIGHT_CHILD(bp) != -1 && GET_BROS(bp) == 0)///////////bp is the first and the only one in this level
        {  
            if  (GET_RIGHT_CHILD(bp) == 0)      //have no right child
            {// it has no right child   
                if (GET_SIZE(HEAD(bp)) > GET_SIZE(HEAD(GET_PART(bp))))  
                    PUT_RIGHT_CHILD(GET_PART(bp),GET_LEFT_CHILD(bp));  
                else  
                    PUT_LEFT_CHILD(GET_PART(bp),GET_LEFT_CHILD(bp)); 
					
                if (GET_LEFT_CHILD(bp) != 0 && GET_PART(bp) != 0)  
                    PUT_PART(GET_LEFT_CHILD(bp),GET_PART(bp));  
            }  
            else if (GET_RIGHT_CHILD(bp) != 0)  //have right child  
            { 
                void *my_tr = GET_RIGHT_CHILD(bp);  
                while(GET_LEFT_CHILD(my_tr) != 0)  
                    my_tr = GET_LEFT_CHILD(my_tr);  
					
                if (GET_SIZE(HEAD(bp)) > GET_SIZE(HEAD(GET_PART(bp))))  
                    PUT_RIGHT_CHILD(GET_PART(bp),my_tr);  
                else  
                    PUT_LEFT_CHILD(GET_PART(bp),my_tr);  
                if (my_tr != GET_RIGHT_CHILD(bp))  
                {  
                    if (GET_RIGHT_CHILD(my_tr) != 0)  
                    {  
                        PUT_LEFT_CHILD(GET_PART(my_tr),GET_RIGHT_CHILD(my_tr));  
                        PUT_LEFT_CHILD(GET_PART(my_tr),GET_RIGHT_CHILD(my_tr));  
                        PUT_PART(GET_RIGHT_CHILD(my_tr),GET_PART(my_tr));  
                    }  
                    else if(GET_RIGHT_CHILD(my_tr) == 0)
                        PUT_LEFT_CHILD(GET_PART(my_tr),0);
                    
                    PUT_RIGHT_CHILD(my_tr,GET_RIGHT_CHILD(bp));  
                    PUT_PART(GET_RIGHT_CHILD(bp),my_tr);  
                }  
                PUT_PART(my_tr,GET_PART(bp));  
                PUT_LEFT_CHILD(my_tr,GET_LEFT_CHILD(bp));  
                if (GET_LEFT_CHILD(bp) != 0)  
                    PUT_PART(GET_LEFT_CHILD(bp),my_tr);  
            }
        }  
    }  
}  
  



// Check one address,  check it block bp and its next block
static void checkblock(void *bp)
{
	
    printf("HEAD[size|allocated]---FOOT[size|allocated]\n");
    printf("%p[%d|%d] --- %p[%d|%d]\n",HEAD(bp),GET_SIZE(HEAD(bp)),GET_ALLOC(HEAD(bp))\
           ,FOOT(bp),GET_SIZE(HEAD(bp)),GET_ALLOC(HEAD(bp)));
    
}

// Check the whole heap
static void mm_check()
{
    void *heap = 0;
    heap = heap_listp;
    while (1)
    {
        printf("%p[%d|%d] --- %p[%d|%d]\n",HEAD(heap),GET_SIZE(HEAD(heap))\
               ,GET_ALLOC(HEAD(heap)),FOOT(heap),GET_SIZE(HEAD(heap)),GET_ALLOC(HEAD(heap)));
        heap = NEXT_BLKP(heap);
		// When reach the end of the heap, then stop
        if (GET_SIZE(HEAD(heap)) == 0 && GET_ALLOC(HEAD(heap)) == 1)
        {
            printf("\n");
            break;
        }  
    }  
}