/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>


#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Bois",
    /* First member's full name */
    "Prikshet",
    /* First member's email address */
    "pshar10@u.rochester.edu",
    /* Second member's full name (leave blank if none) */
    "Nathan McCloud",
    /* Second member's email address (leave blank if none) */
    "nmccloud@u.rochester.edu"
};

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define WSIZE 4
#define DSIZE 8
//given block pointer, compute address of its header and footer.
#define HDRP(bp)    ((char*)(bp) - WSIZE)

#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define CHUNKSIZE (1<<12)
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET(p) (*(unsigned int *) (p))
#define GET_SIZE(p) (GET(p) && ~0x7)

#define GET_ALLOC(p)        (GET(p) & 0x1)
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

//for reading and writing pointers at memory address p.
#define GET_PTR(p) ((unsigned int *)(long)(GET(p)))
#define PUT_PTR(p, ptr) (*(unsigned int *)(p) = ((long)ptr))

#define PACK(size, alloc) ((size) | (alloc))

#define LISTS_NUM 18 // number of lists.
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
//free list implementation
#define S1 (1<<4)
#define S2 (1<<5)
#define S3 (1<<6)
#define S4 (1<<7)
#define S5 (1<<8)
#define S6 (1<<9)
#define S7 (1<<10)
#define S8 (1<<11)
#define S9 (1<<12)
#define S10 (1<<13)
#define S11 (1<<14)
#define S12 (1<<15)
#define S13 (1<<16)
#define S14 (1<<17)
#define S15 (1<<18)
#define S16 (1<<19)
#define S17 (1<<20)
static char *heap_listp;

int getListOffset(size_t size)
{
    if (size <= S1) {
        return 0;
    } else if (size <= S2) {
        return 1;
    } else if (size <= S3) {
        return 2;
    } else if (size <= S4) {
        return 3;
    } else if (size <= S5) {
        return 4;
    } else if (size <= S6) {
        return 5;
    } else if (size <= S7) {
        return 6;
    } else if (size <= S8) {
        return 7;
    } else if (size <= S9) {
        return 8;
    } else if (size <= S10) {
        return 9;
    } else if (size <= S11) {
        return 10;
    } else if (size <= S12) {
        return 11;
    } else if (size <= S13) {
        return 12;
    } else if (size <= S14) {
        return 13;
    } else if (size <= S15) {
        return 14;
    } else if (size <= S16) {
        return 15;
    } else if (size <= S17) {
        return 16;
    } else {
        return 17;
    }
}

void insert_list(void *bp) {
    int index;
    size_t size;
    size = GET_SIZE(HDRP(bp));
    index = getListOffset(size);

    if (GET_PTR(heap_listp + WSIZE * index) == NULL) {
        PUT_PTR(heap_listp + WSIZE * index, bp);
        PUT_PTR(bp, NULL);
        PUT_PTR((unsigned int *)bp + 1, NULL);
    } else {
        PUT_PTR(bp, GET_PTR(heap_listp + WSIZE * index));
        PUT_PTR(GET_PTR(heap_listp + WSIZE * index) + 1, bp);  	/* 修改前一个位置 */
        PUT_PTR((unsigned int *)bp + 1, NULL);
        PUT_PTR(heap_listp + WSIZE * index, bp);
    }
}

void delete_list(void *bp)
{
    int index;
    size_t size;
    size = GET_SIZE(HDRP(bp));
    index = getListOffset(size);
    if (GET_PTR(bp) == NULL && GET_PTR((unsigned int *)bp + 1) == NULL) {

        PUT_PTR(heap_listp + WSIZE * index, NULL);
    } else if (GET_PTR(bp) == NULL && GET_PTR((unsigned int *)bp + 1) != NULL) {

        PUT_PTR(GET_PTR((unsigned int *)bp + 1), NULL);
    } else if (GET_PTR(bp) != NULL && GET_PTR((unsigned int *)bp + 1) == NULL){

        PUT_PTR(heap_listp + WSIZE * index, GET_PTR(bp));
        PUT_PTR(GET_PTR(bp) + 1, NULL);
    } else if (GET_PTR(bp) != NULL && GET_PTR((unsigned int *)bp + 1) != NULL) {

        PUT_PTR(GET_PTR((unsigned int *)bp + 1), GET_PTR(bp));
        PUT_PTR(GET_PTR(bp) + 1, GET_PTR((unsigned int*)bp + 1));
    }
}

static void *coalesce(void * bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    //previous and next allocated
    if (prev_alloc && next_alloc)
        return bp;
        //previous allocated next free
    else if (prev_alloc && !next_alloc)
    {
        delete_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

        //previous free, next allocated
    else if (!prev_alloc && next_alloc)
    {
        delete_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

        //next and previous free.
    else
    {
        delete_list(NEXT_BLKP(bp));
        delete_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_list(bp);
    return bp;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) *WSIZE : (words *WSIZE);
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0)); //free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);

}

/* 
 * mm_init - initialize the malloc package.
 */

/* CSAPP Pg. 585*/
int mm_init(void)
{
    char *bp;
    int i;

    /* create the initial empty heap */
    if((heap_listp = mem_sbrk((LISTS_NUM+4)*WSIZE)) == (void*) -1)
        return -1;
    PUT(heap_listp + LISTS_NUM *WSIZE, 0);
    PUT(heap_listp + ((1+LISTS_NUM)*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + ((2+LISTS_NUM)*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + ((3+LISTS_NUM)*WSIZE), PACK(0, 1));

    for(i = 0; i<LISTS_NUM; i++) {
        PUT_PTR(heap_listp +WSIZE*i, NULL);
    }


    //heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if(bp = extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}








static void *find_fit(size_t asize) {
    /* first fit search */
    int index;
    index = getListOffset(asize);
    unsigned int *ptr;

    while (index < 18) {
        ptr = GET_PTR(heap_listp + 4 * index);
        while (ptr != NULL) {
            if (GET_SIZE(HDRP(ptr)) >= asize) {
                return (void *)ptr;
            }
            ptr = GET_PTR(ptr);
        }
        index++;
    }

    return NULL;

}

static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    delete_list(bp);
    if((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        insert_list(bp);

    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if(size == 0)
        return NULL;

    /* Adjusted block size to include overhead and alignment reqs. */
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size +(DSIZE) + (DSIZE-1))/DSIZE);

    /* search the free list for a fit */
    if((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);

}



/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    size_t asize;
    void *oldptr = ptr;
    void *newptr;

    //free
    if (0 == size) {
        free(oldptr);
        return NULL;
    }


    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if (asize == GET_SIZE(HDRP(oldptr))) {
        return oldptr;
    }

    if (asize < GET_SIZE(HDRP(oldptr))) {
        newptr = mm_malloc(size);
        memmove(newptr, oldptr, size);
        mm_free(oldptr);
        return newptr;
    }
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    memmove(newptr, oldptr, size);
    mm_free(oldptr);
    return newptr;
}














