/*
 * mm.c
 *
 * Name:       Yuyang Zhou
 * Student ID: 2000013061
 * E-mail:     2000013061@stu.pku.edu.cn
 * 
 * Homework Idea:
 *  -- Using 14 Doubly Linked List to maintain free block list. 
 *  -- The header of chain is saved in head_p + WSIZE to head_p + 14 * WSIZE
 *  -- Try to add a new kind of block called Tiny BLock, which helps to save
 *     small blocks with higher efficiency. (Which means less trash), but it
 *     is f**king difficult... So I only gives the idea, Gugugu !!!!!
 *  -- Place the payload into an empty block randomly in front/tail, (Can im-
 *     prove util by 2%, but I don't know why)
 *  -- Using relative address instead of pointer, Which helps to reduce the
 *     size of pointer. (Can improve util by 2%)
 * 
 * Blocks info
 * Empty Block(? Byte):           
 * Header           4 Byte(29 bit size + 000) 
 * Next Empty Block 4 Byte
 * Prev Empty Block 4 Byte
 * Unused           ? Byte
 * Footer           4 Byte(29 bit size + 000) 
 * 
 * NonEmpty Block(? Byte):           
 * Header           4 Byte(29 bit size + 001) 
 * Payload          ? Byte
 * Footer           4 Byte(29 bit size + 001)
 *
 * Tiny Block(256 Byte):         
 * Header           4 Byte(29 bit size + 111)
 * Payload          (15 Byte + 0x00000011) * 15
 * Next Tiny Block  4 Byte
 * Prev Tiny Block  4 Byte
 * Footer           4 Byte(29 bit size + 111) 
 * 
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

const size_t WSIZE = 4;                  // Word size
const size_t DSIZE = 8;                  // Double word size
const size_t CHUNKSIZE = (1<<12);        // Heap size initally
const size_t MAXSIZE = (size_t)(1u<<31); // Max size which can put in malloc
const size_t MINEXPANDSIZE = 504;        // Min size to append in expand_heap

#define PUTW(a,b) (*((unsigned*)(a)) = b)
// put size_t b into array a
#define PUTD(a,b) (*((unsigned*)(a)) = ((char *)b - heap_p))
// put pointer b into array a
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
#define NEXTRP(a) (a)
// give empty block b, return pointer of previous block in the chain
#define PREVRP(a) (((char*)(a))+WSIZE)
// give empty block b, return pointer of next block in the chains
static char* heap_p=NULL;
static char* tail_rp=NULL;

void mm_checkheap(int);
/*
 * VAL: give pointer a, return the pointer saved in a(Relative address)
 */
static inline void* VAL(void* a){
    unsigned re=*((unsigned*)a);
    if (!re) return NULL;
    return ((char*) heap_p + re);
}
/*
 * myrand: return an peusdo-random number
 */
unsigned myrand(void){
    static unsigned int v = 19260817 + 19141001;
    v ^= v << 3;
    v ^= v >> 7;
    v ^= v << 11;
    v ^= v >> 17;
    return v;
}
/*
 * Index: gives block size sz, return the index of chain it belongs to
 */
int Index(size_t sz){
    sz = sz - 2 * DSIZE;
    if (sz < 32) return (sz >> 3) + 1;
    int res = 5;
    sz -= 32;
    for (;;){
        sz >>= 1;
        if (sz < 16)
            return res;
        ++ res;
        if (res >= 14)
            return res;
    }
}

/*
 * delete_block: delete empty block bp in the chain
 */
void delete_block(void* bp){
    //int idx = index(SIZE(HEADRP(bp)));
    void* pre = VAL(PREVRP(bp));
    void* nxt = VAL(NEXTRP(bp));
    if (nxt != NULL)
        PUTD(PREVRP(nxt),pre);
    if (pre != NULL)
        PUTD(NEXTRP(pre),nxt);
}

/*
 * add_block: add empty block bp in the chain, and insert to the head
 */
void add_block(void* bp){
    int idx = Index(SIZE(HEADRP(bp)));
    void* pre = (char*) heap_p + idx * WSIZE;  // location of head pointer
    void* nxt = VAL(pre);                      // the first element of chain
    PUTD(PREVRP(bp),pre);
    PUTD(NEXTRP(bp),nxt);
    PUTD(NEXTRP(pre),bp);
    if (nxt != NULL)
        PUTD(PREVRP(nxt),bp);
}

/*
 * imme_combine: combine empty block bp with previous, next empty block,
 * if they exists, and return the pointer of new block. the new block won't 
 * insert into the chain.
 */
void* imme_combine(void* bp){
    size_t prev_alloc=ALLOC(HEADRP(PREVBLK(bp))); // if prevoius one is empty
    size_t next_alloc=ALLOC(HEADRP(NEXTBLK(bp))); // if next one is empty
    if (!ALLOC(HEADRP(bp))) delete_block(bp);
    if (prev_alloc && next_alloc)
        return bp;
    size_t sz = SIZE(HEADRP(bp)) + 
                (prev_alloc ? 0 : SIZE(HEADRP(PREVBLK(bp)))) +
                (next_alloc ? 0 : SIZE(HEADRP(NEXTBLK(bp))));
                                                  // size of new block
    if (!prev_alloc) delete_block(PREVBLK(bp));
    if (!next_alloc) delete_block(NEXTBLK(bp));
    if (!prev_alloc) bp = PREVBLK(bp);
    PUTW(HEADRP(bp),PACK(sz,0));
    PUTW(FOOTRP(bp),PACK(sz,0));
    return bp;
}
/*
 * expand_heap: expand the heap-size by x bytes.
 * return the pointer of block if success, and NULL otherwize
 */
void* expand_heap(size_t x){
    if (x < MINEXPANDSIZE)
        x = MINEXPANDSIZE;
    // The size it expand must greatwr than MINEXPANDSIZE
    size_t sz = (x + DSIZE - 1) / DSIZE * DSIZE;
    if (sz >= MAXSIZE)
        return NULL;
    void* bp = mem_sbrk(sz);
    if (bp == (void*)-1)
        return NULL;
    PUTW(HEADRP(bp),PACK(sz,0));
    PUTW(FOOTRP(bp),PACK(sz,0));
    PUTD(PREVRP(bp),NULL);
    PUTD(NEXTRP(bp),NULL);
    PUTW(HEADRP(NEXTBLK(bp)),PACK(0,1));
    tail_rp = NEXTBLK(bp);
    // Add a block of size sz into the back of heap
    add_block(bp);
    bp = imme_combine(bp);
    add_block(bp);
    return bp;
}
/*
 * first_fit : Find the first empty block in heap, which size isn't 
 * smaller than Dnum * DSIZE bytes, and return NULL if it doesn't exists.
 */
void* first_fit(size_t Dnum){
    int idx = Index(Dnum * DSIZE);
    while (idx <= 14){
        void* bp = VAL((char*)heap_p + idx * WSIZE);
        while (bp != NULL){
            size_t sz = SIZE(HEADRP(bp));
            if (sz >= Dnum * DSIZE && !ALLOC(HEADRP(bp)))
                return bp;
            bp = VAL(NEXTRP(bp));
        }
        idx ++;
    }
    return NULL;
}

/*
 * best_fit : Find an block in heap, which size isn't smaller than 
 * Dnum * DSIZE bytes, and return NULL if it doesn't exists.
 * 
 * Can simply Modify here to change the fitting rules.
 */
void* best_fit(size_t Dnum){
    int idx = Index(Dnum * DSIZE);
    while (idx <= 4){
        void* bp = VAL((char*)heap_p + idx * WSIZE);
        while (bp != NULL){
            size_t sz = SIZE(HEADRP(bp));
            if (sz >= Dnum * DSIZE && !ALLOC(HEADRP(bp)))
                return bp;
            bp = VAL(NEXTRP(bp));
        }
        idx ++;
    }
    while (idx <= 14){
        void* bp = VAL((char*)heap_p + idx * WSIZE);
        void* res = NULL;
        size_t mn_sz = 1u << 31;
        while (bp != NULL){
            size_t sz = SIZE(HEADRP(bp));
            if (sz >= Dnum * DSIZE && !ALLOC(HEADRP(bp)))
                if (mn_sz > sz){
                    mn_sz = sz;
                    res = bp;
                }
            if (mn_sz <= Dnum * DSIZE + 16)
                break;
            bp = VAL(NEXTRP(bp));
        }
        if (res != NULL)
            return res;
        idx ++;
    }
    return NULL;
}

/*
 * place : split the empty block bp, with a new block of Dnum * DSIZE
 * bytes, and an empty block if exists. the new block is always put in front
 */
void place(void* bp,size_t Dnum){
    size_t sz = SIZE(HEADRP(bp));
    //delete_block(bp);
    if (sz - Dnum * DSIZE < 2 * DSIZE){
        PUTW(HEADRP(bp),PACK(sz,1));
        PUTW(FOOTRP(bp),PACK(sz,1));
    }
    else{
        size_t rem = sz - Dnum * DSIZE;
        PUTW(HEADRP(bp),PACK(Dnum * DSIZE, 1));
        PUTW(FOOTRP(bp),PACK(Dnum * DSIZE, 1));
        void* emp_bp = NEXTBLK(bp);
        PUTW(HEADRP(emp_bp),PACK(rem, 0));
        PUTW(FOOTRP(emp_bp),PACK(rem, 0));
        add_block(emp_bp);
    }
}
/*
 * rand_place : split the empty block bp, with a new block of Dnum * DSIZE
 * bytes, and an empty block if exists. Return the pointer to the new block.
 * The new block is placed according to an strange rule. 
 */
void* rand_place(void* bp,size_t Dnum){
    size_t sz = SIZE(HEADRP(bp));
    if (sz - Dnum * DSIZE < 2 * DSIZE){
        PUTW(HEADRP(bp),PACK(sz,1));
        PUTW(FOOTRP(bp),PACK(sz,1));
        return bp;
    }
    else if (0){//SIZE(HEADRP(PREVBLK(bp))) < SIZE(HEADRP(NEXTBLK(bp)))){
        size_t rem = sz - Dnum * DSIZE;
        PUTW(HEADRP(bp),PACK(Dnum * DSIZE, 1));
        PUTW(FOOTRP(bp),PACK(Dnum * DSIZE, 1));
        void* emp_bp = NEXTBLK(bp);
        PUTW(HEADRP(emp_bp),PACK(rem, 0));
        PUTW(FOOTRP(emp_bp),PACK(rem, 0));
        add_block(emp_bp);
        return bp;
    }
    else{
        size_t rem = sz - Dnum * DSIZE;
        PUTW(HEADRP(bp),PACK(rem, 0));
        PUTW(FOOTRP(bp),PACK(rem, 0));
        add_block(bp);
        bp = NEXTBLK(bp);
        PUTW(HEADRP(bp),PACK(Dnum * DSIZE, 1));
        PUTW(FOOTRP(bp),PACK(Dnum * DSIZE, 1));
        return bp;
    }
}
/*
 * mm_init: initialize the heap. return -1 on error, 0 on success.
 */
int mm_init(void){
    heap_p = mem_sbrk(15 * WSIZE + 3 * WSIZE);
    if (heap_p == (void*)-1)
        return -1;
    PUTD(heap_p + 0 * WSIZE, 0);
    PUTD(heap_p + 1 * WSIZE, NULL);
    PUTD(heap_p + 2 * WSIZE, NULL);
    PUTD(heap_p + 3 * WSIZE, NULL);
    PUTD(heap_p + 4 * WSIZE, NULL);
    PUTD(heap_p + 5 * WSIZE, NULL);
    PUTD(heap_p + 6 * WSIZE, NULL);
    PUTD(heap_p + 7 * WSIZE, NULL);
    PUTD(heap_p + 8 * WSIZE, NULL);
    PUTD(heap_p + 9 * WSIZE, NULL);
    PUTD(heap_p + 10 * WSIZE, NULL);
    PUTD(heap_p + 11 * WSIZE, NULL);
    PUTD(heap_p + 12 * WSIZE, NULL);
    PUTD(heap_p + 13 * WSIZE, NULL);
    PUTD(heap_p + 14 * WSIZE, NULL);
    // the head of 14 free block chains.

    PUTW(heap_p + 15 * WSIZE + 0 * WSIZE, PACK(DSIZE,1));
    PUTW(heap_p + 15 * WSIZE + 1 * WSIZE, PACK(DSIZE,1));
    PUTW(heap_p + 15 * WSIZE + 2 * WSIZE, PACK(0,1));
    // The header block and tail block of the heap.

    if (expand_heap(CHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * malloc: create a piece of memory of sz bytes.
 */
void *malloc(size_t sz){
    if (sz == (size_t) 0 || sz > MAXSIZE)
        return NULL;
    size_t Dnum;
    if (sz <= DSIZE)
        Dnum = 2;
    else
        Dnum = (sz + DSIZE + DSIZE - 1) / DSIZE;
    void* bp = best_fit(Dnum);
    if (bp != NULL){
        delete_block(bp);
        bp = rand_place(bp,Dnum);
        return bp;
    }

    bp = expand_heap(Dnum * DSIZE);
    if (bp == NULL)
        return bp;
    delete_block(bp);
    bp = rand_place(bp,Dnum);
    return bp;
}

/*
 * free: free a piece of memory, which is given by pointer ptr
 */
void free(void *ptr){
    if (ptr == NULL)
        return;
    size_t sz = SIZE(HEADRP(ptr));
    PUTW(HEADRP(ptr),PACK(sz,0));
    PUTW(FOOTRP(ptr),PACK(sz,0));
    PUTD(PREVRP(ptr),NULL);
    PUTD(NEXTRP(ptr),NULL);
    add_block(ptr);
    ptr = imme_combine(ptr);
    add_block(ptr);
}
/*
 * realloc - realloc a piece of memory which is given by pointer oldptr,
   resize it with size_t sz, and return the pointer to the new block.
     -- if oldptr == NULL, it's equal to mm_malloc(sz)
     -- if sz == 0, it's equal to mm_free(oldptr)
 */
void *realloc(void *oldptr, size_t sz) {
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
    PUTW(HEADRP(newptr),PACK(newsz,1));
    PUTW(FOOTRP(newptr),PACK(newsz,1));
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
        if (ptr == NULL)
            return NULL;
        memcpy(ptr,newptr,newsz - DSIZE);
        mm_free(newptr);
        return ptr;
    }
    //fprintf(stderr,"Error in realloc!!!\n");
    return NULL;
}


/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size) {
    //printf("Calloc Ins %u\n",size);
    if (!nmemb || !size)
        return NULL;
    if (MAXSIZE / nmemb < size)
        return NULL;
    void* ptr = malloc(nmemb * size);
    memset(ptr, 0, nmemb * size);
    return ptr;
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
 * heap_error: report heap_error with information *s, in line lineno
 */
void heap_error(const char* s,int lineno){
    fprintf(stderr,"%s,",s);
    fprintf(stderr,"in line %d\n",lineno);
    exit(0);
}

#define VALD(a) (*((unsigned*)(a)))
// Return the 4 byte value save in pointer a
#define VIS(a) (*((unsigned*)(a)) & 0x2)
// Return if an empty block is visited by the checker function.

/*
 * valid block: give block pointer bp, and check if it is an valid block
 * check the size of block, whether header and footer matches, and the align
 * of pointer
 */
void valid_block(void* bp,int lineno){
    size_t siz = SIZE(HEADRP(bp));
    size_t alloc = ALLOC(HEADRP(bp));
    if (siz <= 8 || siz >= MAXSIZE || NEXTBLK(bp) > tail_rp)
        heap_error("invalid heap size",lineno);
    if (SIZE(FOOTRP(bp)) != siz || ALLOC(FOOTRP(bp)) != alloc)
        heap_error("unmatching header and footer",lineno);
}

/*
 * mm_checkheap: check the heap if it's valid, and report the error
 * if something wrong happened.
 */
void mm_checkheap(int lineno){
    void* bp = heap_p + 18 * WSIZE;
    
    // check sprologue block
    if (VALD(heap_p + 15 * WSIZE) != PACK(DSIZE,1)||
        VALD(heap_p + 16 * WSIZE) != PACK(DSIZE,1))
        heap_error("broken prologue block",lineno);
    
    // check the heap block chain
    int empty_num = 0;
    int pref_alloc = 1;
    for (;;){
        size_t siz = SIZE(HEADRP(bp));
        size_t alloc = ALLOC(HEADRP(bp));
        if (siz == 0 && alloc == 1){
            if (bp == tail_rp) break;
            else heap_error("unexpected ending of heap",lineno);
        }
        valid_block(bp, lineno);
        if (!alloc){
            if (!pref_alloc)
                heap_error("two consecutive free blocks",lineno);
            ++empty_num;
            PUTW(HEADRP(bp),PACK(siz,2));
            PUTW(FOOTRP(bp),PACK(siz,2));
        }
        bp = NEXTBLK(bp);
        pref_alloc = alloc;
    }
    bp = heap_p + WSIZE;

    //check the free list chain
    for (;bp != heap_p + 15 * WSIZE; bp = (char*)(bp) + WSIZE){
        void* now = bp;
        void* ptr = VAL(NEXTRP(bp));
        for (;;){
            if (ptr == NULL) break;
            if (!in_heap(ptr) || !aligned(ptr))
                heap_error("invalid free list pointer",lineno);
            valid_block(ptr, lineno);
            if (VAL(PREVRP(ptr)) != now)
                heap_error("unmatch free list pointer",lineno);
            if (!VIS(HEADRP(ptr)))
                heap_error("free list forms an ring",lineno);
            size_t siz = SIZE(HEADRP(ptr));
            if (bp != (char*)(heap_p) + Index(siz) * WSIZE)
                heap_error("block in wrong free list",lineno);
            PUTW(HEADRP(ptr),PACK(siz,0));
            PUTW(FOOTRP(ptr),PACK(siz,0));
            void* nxt = VAL(NEXTRP(ptr));
            now = ptr;
            ptr = nxt;
            --empty_num;
        }
    }
    if (empty_num)
        heap_error("some block not in free list",lineno);
}