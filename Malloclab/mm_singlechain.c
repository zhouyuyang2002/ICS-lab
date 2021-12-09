/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)

const size_t WSIZE=4; 
const size_t DSIZE=8;
const size_t CHUNKSIZE=(1<<12);
const size_t MAXSIZE=(size_t)(1u<<31);

#define PUT(a,b) (*((unsigned*)(a)) = (b))
// put b into array a
#define PACK(a,b) ((size_t)(a)|(size_t)(b))
// pack memory a & alloc b together
#define SIZE(a) (*((unsigned*)(a)) & 0xfffffff8)
// get memory size of header/footer a
#define ALLOC(a) (*((unsigned*)(a)) & 0x1)
// get allocation of block a
#define HEADRP(a) ((char*)(a)-WSIZE)
// give memory a, return header of a
#define FOOTRP(a) ((char*)(a)-DSIZE+SIZE(HEADRP(a)))
// give memory a, return footer of a
#define PREVBLK(a) ((char*)(a)-SIZE((char*)(a)-DSIZE))
// give memory a, return pointer of previous block
#define NEXTBLK(a) ((char*)(a)+SIZE(HEADRP(a)))
// give memory a, return pointer of previous block
static char* heap_p=NULL;


/*
 * imme_combine: combine empty block bp with previous, next empty block,
 * if they exists, and return thre pointer of new block.
 */
void* imme_combine(void* bp){
    //fprintf(stderr,"Combine pointer = %x\n",bp);
    size_t prev_alloc=ALLOC(HEADRP(PREVBLK(bp)));
    size_t next_alloc=ALLOC(HEADRP(NEXTBLK(bp)));
    //fprintf(stderr,"pre = %x,nxt = %x\n",prev_alloc,next_alloc);
    if (prev_alloc && next_alloc)
        return bp;
    size_t sz = SIZE(HEADRP(bp)) + 
                (prev_alloc ? 0 : SIZE(HEADRP(PREVBLK(bp)))) +
                (next_alloc ? 0 : SIZE(HEADRP(NEXTBLK(bp))));
    //fprintf(stderr,"new_sz = %x,pre_sz = %x,nxt_sz = %x\n",SIZE(HEADRP(bp)),SIZE(HEADRP(PREVBLK(bp))),SIZE(HEADRP(NEXTBLK(bp))));
    if (!prev_alloc)
        bp = PREVBLK(bp);
    PUT(HEADRP(bp),PACK(sz,0));
    PUT(FOOTRP(bp),PACK(sz,0));
    return bp;
}
/*
 * expand_heap: expand the heap-size by x bytes.
 * return NULL if unsuccess, and the pointer of block otherwize
 */
void* expand_heap(size_t x){
    //fprintf(stderr,"expand_heap size = %x\n",x);
    size_t sz = (x + DSIZE - 1) / DSIZE * DSIZE;
    if (sz >= MAXSIZE)
        return NULL;
    void* bp = mem_sbrk(sz);
    if (bp == (void*)-1)
        return NULL;
    //fprintf(stderr,"expand_heap size = %x\n",sz);
    PUT(HEADRP(bp),PACK(sz,0));
    PUT(FOOTRP(bp),PACK(sz,0));
    PUT(HEADRP(NEXTBLK(bp)),PACK(0,1));
    return imme_combine(bp);
}
/*
 * first_fit : Find the first empty block in heap, which size is 
 * greater than Dnum * DSIZE bytes, and return NULL if it doesn't exists.
 */
void* first_fit(size_t Dnum){
    void* bp = heap_p;
    size_t sz;
    while (1){
        sz = SIZE(HEADRP(bp));
        if (!sz) break;
        if (sz >= Dnum * 8 && !ALLOC(HEADRP(bp)))
            return bp;
        bp = NEXTBLK(bp);
    }
    return NULL;
}
/*
 * place : split the empty block bp, with a new block of Dnum * DSIZE
 * bytes, and an empty block if exists.
 */
void place(void* bp,size_t Dnum){
    size_t sz = SIZE(HEADRP(bp));
    if (sz - Dnum * DSIZE < 2 * DSIZE){
        PUT(HEADRP(bp),PACK(sz,1));
        PUT(FOOTRP(bp),PACK(sz,1));
    }
    else{
        size_t rem = sz - Dnum * DSIZE;
        PUT(HEADRP(bp),PACK(Dnum * DSIZE, 1));
        PUT(FOOTRP(bp),PACK(Dnum * DSIZE, 1));
        bp = NEXTBLK(bp);
        PUT(HEADRP(bp),PACK(rem, 0));
        PUT(FOOTRP(bp),PACK(rem, 0));
    }
}
/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void){
    heap_p = mem_sbrk(4 * WSIZE);
    if (heap_p == (void*)-1)
        return -1;
    PUT(heap_p + 0 * WSIZE, 0);
    PUT(heap_p + 1 * WSIZE, PACK(DSIZE,1));
    PUT(heap_p + 2 * WSIZE, PACK(DSIZE,1));
    PUT(heap_p + 3 * WSIZE, PACK(0,1));
    heap_p = heap_p + DSIZE;
    if (expand_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * malloc
 */
void *malloc(size_t sz){
    //fprintf(stderr,"malloc Ins size = %x\n",sz);
    if (sz == (size_t) 0 || sz > MAXSIZE)
        return NULL;
    size_t Dnum;
    if (sz <= DSIZE)
        Dnum = 2;
    else
        Dnum = (sz + DSIZE + DSIZE -1) / DSIZE;
    void* bp = first_fit(Dnum);
    //fprintf(stderr,"malloc pointer = %x\n",bp);
    if (bp != NULL){
        place(bp,Dnum);
        return bp;
    }

    bp = expand_heap(Dnum * DSIZE);
    if (bp == NULL)
        return bp;
    place(bp,Dnum);
    return bp;
}

/*
 * free
 */
void free (void *ptr){
    //fprintf(stderr,"Free Ins pointer = %x\n",ptr);
    if (ptr == NULL)
        return;
    size_t sz = SIZE(HEADRP(ptr));
    PUT(HEADRP(ptr),PACK(sz,0));
    PUT(FOOTRP(ptr),PACK(sz,0));
    imme_combine(ptr);
}
/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t sz) {
    //fprintf(stderr,"Realloc Ins pointer = %x, size_t = %u\n",oldptr,sz);
    if (oldptr == NULL)
        return mm_malloc(sz);
    if (!sz){
        mm_free(oldptr);
        return NULL;
    }
    
    size_t Dnum;
    if (sz <= DSIZE)
        Dnum = 2;
    else
        Dnum = (sz + DSIZE + DSIZE - 1) / DSIZE;
    void* newptr = imme_combine(oldptr);
    size_t newsz = SIZE(HEADRP(newptr));
    //fprintf(stderr,"Realloc Ins pointer = %x, size_t = %x, new_sz = %x, Dnum = %x\n",oldptr,sz,newsz,Dnum);
    PUT(HEADRP(newptr),PACK(newsz,1));
    PUT(FOOTRP(newptr),PACK(newsz,1));
    if (newptr != oldptr)
        memcpy(newptr,oldptr,SIZE(HEADRP(oldptr)));
    if (Dnum * DSIZE == newsz)
        return newptr;

    if (Dnum * DSIZE < newsz){
        place(newptr,Dnum);
        return newptr;
    }
    if (Dnum * DSIZE > newsz){
        void* ptr = mm_malloc(sz);
        //fprintf(stderr,"Realloc Malloc pointer = %x,block size = %x",ptr,SIZE(HEADRP(ptr)));
        if (ptr == NULL)
            return NULL;
        memcpy(ptr,newptr,newsz - DSIZE);
        mm_free(newptr);
        return ptr;
    }
    fprintf(stderr,"Error in realloc!!!\n");
    return NULL;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    //printf("Calloc Ins %u\n",size);
    if (!nmemb || !size)
        return NULL;
    if (MAXSIZE / nmemb < size)
        return NULL;
    return malloc(nmemb * size);
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
}
