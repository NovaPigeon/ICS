/*
 * mm.c
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 *
 */
/* 此版本为显式空闲链表（去脚部、压指针） LIFO ，首次适配
 */
// 2100012945 寿晨宸

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
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
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
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT - 1)) & ~0x7)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* 包装块的头部和脚部，包含前一个块和当前块的分配信息以及当前块的大小 */
#define PACK(size, alloc_pred, alloc_curr) ((size) | (alloc_pred << 1) | (alloc_curr))

/* 将指针的真实值与堆的初始地址相减，可以将指针压缩至 4 bytes */
/* 获取真实的指针值 */
#define TRUEP(p) ((void *)(((unsigned long)(p)) + ((unsigned long)(mm_heap_lo))))

/* 压缩指针大小 */
#define ZIPP(p) ((unsigned int)(((unsigned long)(p)) - ((unsigned long)(mm_heap_lo))))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* 读和写 pred 和 succ 指针（以 4 bytes 的形式）*/
/* 由于只在此处压指针，故只在此处做指针的转换，其他地方照常使用指针 */
#define GET_PRED(p) (TRUEP(*(unsigned int *)(p)))
#define GET_SUCC(p) (TRUEP(*(unsigned int *)(((char *)(p)) + WSIZE)))
#define SET_PRED(p, ptr) (SET_PTR(p, ZIPP(ptr)))
#define SET_SUCC(p, ptr) (SET_PTR((((char *)(p)) + WSIZE), ZIPP(ptr)))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC_PREV(p) ((GET(p) & 0x2) >> 1)
#define GET_ALLOC_CURR(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
/* 注意，已分配块无脚部 */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static char *heap_listp = NULL; /* Pointer to first block */
static char *heap_tailp = NULL; /* 指向堆的结尾 */
static void *mm_heap_lo;

static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *ptr);
static void delete_free(void *ptr);
static void instert_free(void *ptr);
/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void)
{
    /*
        显式空间链表的初始结构为，对齐块：4字节 0/1/1，序言块：16字节，16/1/1，pred 为 NULL，succ 为NULL
        结尾块：4 字节，0/1/1
    */
    mm_heap_lo = mem_heap_lo();
    if ((heap_listp = mem_sbrk(8 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, PACK(0, 1, 0));                // 对齐块
    PUT(heap_listp + (1 * WSIZE), PACK(16, 0, 1)); // 序言块的头部
    PUT(heap_listp + (4 * WSIZE), PACK(16, 0, 1));
    heap_listp = heap_listp + (DSIZE);
    heap_tailp = NEXT_BLKP(heap_listp); // 将尾部移动到序言块的下一个块
    SET_PRED(heap_listp, mm_heap_lo);   // 序言块pred为NULL
    SET_SUCC(heap_listp, mm_heap_lo);   // 序言块succ为NULL
    // printf("\n%lu\n\n",(unsigned long)mm_heap_lo);
    PUT(HDRP(heap_tailp), PACK(0, 1, 1)); // 结尾块头部
    PUT(heap_tailp, PACK(0, 1, 1));
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}
/*
 * malloc
 */
void *malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    if (heap_listp == NULL)
    {
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= (3 * WSIZE)) // 空闲块的大小最小为16，转换为已分配块后，已分配块的有效载荷最小为16-4=12
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        // dbg_printf("fit malloc %lu => %p\n", asize, bp);
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    // printf("\nextend malloc init succ:%lu\n\n",(unsigned long)GET_SUCC(bp));
    place(bp, asize);
    // dbg_printf("extend malloc %lu => %p\n", asize, bp);
    return bp;
}

/*
 * free
 */
void free(void *ptr)
{
    // printf("mm_free => ptr: %lu\n",(unsigned long)ptr);
    //  按地址顺序来组织空闲链表
    //  遍历空闲链表，找到第一个大于当前块地址的空闲块，将当前块插入链表
    if (ptr == 0)
        return;

    if (heap_listp == NULL)
    {
        mm_init();
    }
    size_t size = GET_SIZE(HDRP(ptr));
    int pred_alloc = GET_ALLOC_PREV(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, pred_alloc, 0)); // 将当前块设置为空闲块
    PUT(FTRP(ptr), PACK(size, pred_alloc, 0));
    // 设置succ和pred指针,插入链表头部
    instert_free(ptr);
    // 因为当前块的状态改变，故下个块的头部和脚部也要改变
    void *next_ptr = NEXT_BLKP(ptr);
    size_t next_size = GET_SIZE(HDRP(next_ptr));
    size_t next_alloc = GET_ALLOC_CURR(HDRP(next_ptr));
    PUT(HDRP(next_ptr), PACK(next_size, 0, next_alloc));
    if (next_alloc == 0)
        PUT(FTRP(next_ptr), PACK(next_size, 0, next_alloc));
    coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *ptr, size_t size)
{
    size_t old_size;
    size_t new_size;
    // size_t next_alloc;
    size_t prev_alloc;
    // size_t next_size;
    // void* next_ptr;
    void *rest_ptr;
    void *new_ptr;
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0)
    {
        free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
    {
        return malloc(size);
    }
    if (!GET_ALLOC_CURR(HDRP(ptr)))
        return 0;
    if (size <= (3 * WSIZE)) // 空闲块的大小最小为16，转换为已分配块后，已分配块的有效载荷最小为16-4=12
        new_size = 2 * DSIZE;
    else
        new_size = DSIZE * ((size + (WSIZE) + (DSIZE - 1)) / DSIZE);
    old_size = GET_SIZE(HDRP(ptr));

    // 在处理过程中尽量利用原有的块，并减少拷贝操作
    if (old_size == new_size)
        new_ptr = ptr;            // 若相同，则不必改变
    else if (old_size > new_size) // 若新分配的块大小小于旧的块大小，则在旧的块处就地分割。
    {
        // 因为old_size和new_size都做了对齐处理，所以分割后的剩余块不必再做对齐处理
        // 空闲块最小为16bytes
        if (old_size - new_size >= (4 * WSIZE))
        {
            // 处理下一块的头部和脚部
            /*
            next_ptr=NEXT_BLKP(ptr);
            next_alloc=GET_ALLOC_CURR(HDRP(next_ptr));
            next_size=GET_SIZE(HDRP(next_ptr));
            PUT(HDRP(next_ptr),PACK(next_size,0,next_alloc));
            if(next_alloc==0)
                PUT(FTRP(next_ptr),PACK(next_size,0,next_alloc));
                */
            // 处理当前块
            prev_alloc = GET_ALLOC_PREV(HDRP(ptr));
            PUT(HDRP(ptr), PACK(new_size, prev_alloc, 1));
            // 处理剩余块
            rest_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(rest_ptr), PACK(old_size - new_size, 1, 1));
            // PUT(FTRP(rest_ptr),PACK(old_size-new_size,1,0));
            // 将分割出的空闲块插入链表中
            /*
            char *p = GET_SUCC(heap_listp); // 从头开始便历
            for (; (void *)p != (void *)heap_tailp; p = GET_SUCC(p))
            {
                if (rest_ptr < (void *)p)
                {
                    void *pred_ptr = GET_PRED(p);
                    SET_SUCC(pred_ptr, ptr); // 设置succ和pred指针
                    SET_PRED(p, ptr);
                    SET_PRED(ptr, pred_ptr);
                    SET_SUCC(ptr, p);
                    break;
                }
            }
            //合并空闲块

            coalesce(rest_ptr);
            */
            free(rest_ptr);
            new_ptr = ptr;
        }
        else
        {
            // 否则，分配新的块，将原本块的内容复制到新的块中，并将原本的块设置为空闲
            /*
            new_ptr=malloc(new_size);
            memcpy(new_ptr,ptr,new_size);
            free(ptr);
            */
            new_ptr = ptr;
        }
    }
    else
    {
        // 若新分配的块大小大于旧的块大小
        new_ptr = malloc(new_size);
        memcpy(new_ptr, ptr, new_size);
        free(ptr);
    }
    return new_ptr;
    // dbg_printf("realloc %lu => %p\n", new_size, new_ptr);
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc(size_t nmemb, size_t size)
{
    return NULL;
}

/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p)
{
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p)
{
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno)
{
    void *p = heap_listp;
    size_t size = GET_SIZE(HDRP(p)), prev_alloc = GET_ALLOC_PREV(HDRP(p)), curr_alloc = GET_ALLOC_CURR(HDRP(p));
    unsigned long pred = (unsigned long)GET_PRED(p), succ = (unsigned long)GET_SUCC(p), add = (unsigned long)(p);
    printf("add:%lu ;size:%lu ;prev_alloc:%lu ;curr_alloc:%lu ;pred:%lu ;succ %lu\n",
           add, size, prev_alloc, curr_alloc, pred, succ);
    p = NEXT_BLKP(p);
    for (; (void *)p != (void *)heap_tailp; p = NEXT_BLKP(p))
    {
        size_t size = GET_SIZE(HDRP(p)), prev_alloc = GET_ALLOC_PREV(HDRP(p)), curr_alloc = GET_ALLOC_CURR(HDRP(p));
        add = (unsigned long)(p);
        if (curr_alloc == 1)
            printf("add:%lu ;size:%lu ;prev_alloc:%lu ;curr_alloc:%lu;\n",
                   add, size, prev_alloc, curr_alloc);
        else
        {
            unsigned long pred = (unsigned long)GET_PRED(p), succ = (unsigned long)GET_SUCC(p);
            add = (unsigned long)(p);
            printf("add:%lu ;size:%lu ;prev_alloc:%lu ;curr_alloc:%lu ;pred:%lu ;succ %lu\n",
                   add, size, prev_alloc, curr_alloc, pred, succ);
        }
    }
    printf("\n\n");
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    // size*=2;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        // dbg_printf("extend error %lu\n", size);
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    // 在原本的结尾块处插入大小为size的空闲块，并设置新的结尾块
    // 此过程中，当前空闲块的 pred 和 上一块的 succ 指针不变
    char *p = heap_tailp;
    int pred_alloc = GET_ALLOC_PREV(HDRP(heap_tailp)); // 保存上一块的分配情况
    PUT(HDRP(p), PACK(size, pred_alloc, 0));           // 空闲块头部
    PUT(FTRP(p), PACK(size, pred_alloc, 0));           // 空闲块脚部
    heap_tailp = NEXT_BLKP(p);
    //将新扩展的块放在链表头部
    instert_free(p);
    PUT(HDRP(heap_tailp), PACK(0, 0, 1)); // 结尾块的头部
    PUT(heap_tailp, PACK(0, 1, 1));

    /* Coalesce if the previous block was free */
    return coalesce(p);
}

static void *coalesce(void *ptr)
{
    // 把合并后的块放在链表起始位置,并将原来的块从链表中删除
    void *next_ptr = NEXT_BLKP(ptr);
    void *res_ptr;
    size_t prev_alloc = GET_ALLOC_PREV(HDRP(ptr));
    size_t next_alloc = GET_ALLOC_CURR(HDRP(next_ptr));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc)
    { /* Case 1 */
        return ptr;
    }

    else if (prev_alloc && !next_alloc)
    { /* Case 2 */
        size += GET_SIZE(HDRP(next_ptr));
        delete_free(ptr);
        delete_free(next_ptr);
        PUT(HDRP(ptr), PACK(size, prev_alloc, 0));
        PUT(FTRP(ptr), PACK(size, prev_alloc, 0));
        res_ptr = ptr;
    }

    else if (!prev_alloc && next_alloc)
    { /* Case 3 */
        void *prev_ptr = PREV_BLKP(ptr);
        size_t pp_alloc = GET_ALLOC_PREV(HDRP(prev_ptr));
        size += GET_SIZE(HDRP(prev_ptr));
        delete_free(prev_ptr);
        delete_free(ptr);
        PUT(HDRP(prev_ptr), PACK(size, pp_alloc, 0));
        PUT(FTRP(prev_ptr), PACK(size, pp_alloc, 0));
        res_ptr = prev_ptr;
    }
    else
    {
        /* Case 4 */
        void *prev_ptr = PREV_BLKP(ptr);
        size += (GET_SIZE(HDRP(prev_ptr)) + GET_SIZE(HDRP(next_ptr)));
        size_t pp_alloc = GET_ALLOC_PREV(HDRP(prev_ptr));
        delete_free(prev_ptr);
        delete_free(ptr);
        delete_free(next_ptr);
        PUT(HDRP(prev_ptr), PACK(size, pp_alloc, 0));
        PUT(FTRP(prev_ptr), PACK(size, pp_alloc, 0));
        res_ptr = prev_ptr;
    }
    char *bp = GET_SUCC(heap_listp);
    SET_PRED(res_ptr, heap_listp);
    SET_SUCC(heap_listp, res_ptr);
    SET_SUCC(res_ptr, bp);
    if (bp != mm_heap_lo)
        SET_PRED(bp, res_ptr);
    return res_ptr;
}
static void *find_fit(size_t asize)
{
    // 首次适配
    void *bp;
    for (bp = GET_SUCC(heap_listp); bp != mm_heap_lo; bp = GET_SUCC(bp))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
            return bp;
    }
    return NULL;
}
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    void *pred_ptr = GET_PRED(bp);
    void *succ_ptr = GET_SUCC(bp);
    void *next_ptr = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC_PREV(HDRP(bp));
    // 空闲块最小为 16bytes，若分割后剩余部分大于 16 bytes，则分配空闲块，否则，把剩余部分当作填充物
    if (csize - asize >= 4 * WSIZE)
    {
        PUT(HDRP(bp), PACK(asize, prev_alloc, 1)); // 只分配头部即可
        void *rest_ptr = NEXT_BLKP(bp);            // 处理剩余块
        SET_PRED(succ_ptr, pred_ptr);
        SET_SUCC(pred_ptr, succ_ptr);
        char *hp = GET_SUCC(heap_listp);
        if (hp != mm_heap_lo)
            SET_PRED(hp, rest_ptr);
        SET_SUCC(rest_ptr, hp);
        SET_SUCC(heap_listp, rest_ptr);
        SET_PRED(rest_ptr, heap_listp);
        PUT(HDRP(rest_ptr), PACK(csize - asize, 1, 0));
        PUT(FTRP(rest_ptr), PACK(csize - asize, 1, 0));
    }
    else
    {
        // 将空闲块从链表中删除,并为当前块设置头部
        SET_PRED(succ_ptr, pred_ptr);
        SET_SUCC(pred_ptr, succ_ptr);
        PUT(HDRP(bp), PACK(csize, prev_alloc, 1));
        // 设置下一块的头部和脚部
        size_t next_size = GET_SIZE(HDRP(next_ptr));
        size_t next_alloc = GET_ALLOC_CURR(HDRP(next_ptr));
        PUT(HDRP(next_ptr), PACK(next_size, 1, next_alloc));
        if (next_alloc == 0)
            PUT(FTRP(next_ptr), PACK(next_size, 1, next_alloc));
    }
}
static void delete_free(void *ptr)
{
    void *pred_ptr = GET_PRED(ptr);
    void *succ_ptr = GET_SUCC(ptr);
    if (succ_ptr != mm_heap_lo)
        SET_PRED(succ_ptr, pred_ptr);
    SET_SUCC(pred_ptr, succ_ptr);
}
static void instert_free(void *ptr)
{
    void *succ_ptr = GET_SUCC(heap_listp);
    if (succ_ptr != mm_heap_lo)
        SET_PRED(succ_ptr, ptr);
    SET_SUCC(ptr, succ_ptr);
    SET_PRED(ptr, heap_listp);
    SET_SUCC(heap_listp, ptr);
}