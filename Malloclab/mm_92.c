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
const size_t MINEXPANDSIZE=(1<<8);
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

/*
Empty Block:         NonEmpty Block   
Header     4 Byte    Header     4 Byte
Next Block 8 Byte    Payload
Prev Block 8 Byte
Unused     
Footer     4 Byte    Footer     4 Byte
*/

/*
 * index : return the index of chain according to size_t sz
 * Mapping Rule: 1 -> {1~16} + 8    2 -> {17~32} + 8
 *               3 -> {33~64} + 8   4 -> {65~128} + 8    5 -> {129~256} + 8
 *               6 -> {257~512} + 8 7 -> {513~1024} + 8  8 -> {1025~2048} + 8
 *               9 -> {2049~} + 8 
 */

static inline void* VAL(void* a){
    unsigned re=*((unsigned*)a);
    if (!re) return NULL;
    return ((char*) heap_p + re);
}
unsigned myrand(){
    static unsigned int v = 19260817 + 19141001;
    v ^= v << 3;
    v ^= v >> 7;
    v ^= v << 11;
    v ^= v >> 17;
    return v;
}
int Index(size_t sz){
    sz = sz - DSIZE;
    if (sz > (1 << 12)) return 10;
    int res = 1;
    for (;;){
        sz = sz >> 1;
        if (sz <= 8)
            return res;
        ++ res;
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
    //fprintf(stderr,"pointer = %lx,siz = %x\n",bp,SIZE(HEADRP(bp)));
    PUTD(NEXTRP(pre),bp);
    if (nxt != NULL)
        PUTD(PREVRP(nxt),bp);
}


/*
 * imme_combine: combine empty block bp with previous, next empty block,
 * if they exists, and return thre pointer of new block.
 */
void* imme_combine(void* bp){
    //fprintf(stderr,"Combine pointer = %x\n",bp);
    size_t prev_alloc=ALLOC(HEADRP(PREVBLK(bp)));
    size_t next_alloc=ALLOC(HEADRP(NEXTBLK(bp)));
    //fprintf(stderr,"pre = %x,nxt = %x\n",prev_alloc,next_alloc);
    if (!ALLOC(HEADRP(bp))) delete_block(bp);
    if (prev_alloc && next_alloc)
        return bp;
    size_t sz = SIZE(HEADRP(bp)) + 
                (prev_alloc ? 0 : SIZE(HEADRP(PREVBLK(bp)))) +
                (next_alloc ? 0 : SIZE(HEADRP(NEXTBLK(bp))));
    if (!prev_alloc) delete_block(PREVBLK(bp));
    if (!next_alloc) delete_block(NEXTBLK(bp));
    if (!prev_alloc) bp = PREVBLK(bp);
    PUTW(HEADRP(bp),PACK(sz,0));
    PUTW(FOOTRP(bp),PACK(sz,0));
    return bp;
}
/*
 * expand_heap: expand the heap-size by x bytes.
 * return NULL if unsuccess, and the pointer of block otherwize
 */
void* expand_heap(size_t x){
    if (x < MINEXPANDSIZE)
        x = MINEXPANDSIZE;
    size_t sz = (x + DSIZE - 1) / DSIZE * DSIZE;
    if (sz >= MAXSIZE)
        return NULL;
    void* bp = mem_sbrk(sz);
    if (bp == (void*)-1)
        return NULL;
    PUTW(HEADRP(bp),PACK(sz,0)); // 
    PUTW(FOOTRP(bp),PACK(sz,0));
    PUTD(PREVRP(bp),NULL);
    PUTD(NEXTRP(bp),NULL);
    PUTW(HEADRP(NEXTBLK(bp)),PACK(0,1));
    add_block(bp);
    bp = imme_combine(bp);
    add_block(bp);
    return bp;
}
/*
 * first_fit : Find the first empty block in heap, which size is 
 * greater than Dnum * DSIZE bytes, and return NULL if it doesn't exists.
 */
void* first_fit(size_t Dnum){
    int idx = Index(Dnum * DSIZE);
    while (idx <= 10){
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
 * best_fit : Find the first empty block in heap, which size is 
 * greater than Dnum * DSIZE bytes, and return NULL if it doesn't exists.
 */
void* best_fit(size_t Dnum){
    return first_fit(Dnum);
}
/*
 * place : split the empty block bp, with a new block of Dnum * DSIZE
 * bytes, and an empty block if exists.
 */
void place(void* bp,size_t Dnum){
    size_t sz = SIZE(HEADRP(bp));
    //delete_block(bp);
    if (sz - Dnum * DSIZE < 3 * DSIZE){
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
 * bytes, and an empty block if exists.
 */
void* rand_place(void* bp,size_t Dnum){
    size_t sz = SIZE(HEADRP(bp));
    //delete_block(bp);
    if (sz - Dnum * DSIZE < 3 * DSIZE){
        PUTW(HEADRP(bp),PACK(sz,1));
        PUTW(FOOTRP(bp),PACK(sz,1));
        return bp;
    }
    else if (myrand()&1){
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
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void){
    heap_p = mem_sbrk(11 * WSIZE + 3 * WSIZE);
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
    PUTW(heap_p + 11 * WSIZE + 0 * WSIZE, PACK(DSIZE,1));
    PUTW(heap_p + 11 * WSIZE + 1 * WSIZE, PACK(DSIZE,1));
    PUTW(heap_p + 11 * WSIZE + 2 * WSIZE, PACK(0,1));
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
        Dnum = (sz + DSIZE + DSIZE - 1) / DSIZE;
    void* bp = best_fit(Dnum);
    //fprintf(stderr,"malloc pointer = %lx\n",bp);
    if (bp != NULL){
        delete_block(bp);
        bp = rand_place(bp,Dnum);
        return bp;
    }

    bp = expand_heap(Dnum * DSIZE);
    if (bp == NULL)
        return bp;
    delete_block(bp);
    //fprintf(stderr,"malloc pr = %lx\n",
    bp = rand_place(bp,Dnum);
    return bp;
}

/*
 * free
 */
void free(void *ptr){
    //fprintf(stderr,"Free Ins pointer = %lx\n",ptr);
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
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t sz) {
    //fprintf(stderr,"Realloc Ins pointer = %lx, size_t = %x\n",oldptr,sz);
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
