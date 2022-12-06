/*
 * mm.c
 *
 * 寿晨宸 2100012945
 *
 * 此版本以显式空闲链表为基础，采用了分离适配的空闲链表结构
 *
 * 堆的总体结构如下：
 * ——————————————————————————————————————
 * CLASS_NUM 个 8 字节指针，作为每组链表的基指针
 * ——————————————————————————————————————
 * 4 字节对齐块
 * ——————————————————————————————————————
 * 8 字节序言块（8/1/1 4字节头部，4字节脚部）
 * ——————————————————————————————————————
 * 堆内的块
 * ——————————————————————————————————————
 * 结尾块（0/x/1，无脚部）
 * ——————————————————————————————————————
 *
 * 在设计过程中，采用了去脚部、压指针等技术，
 * 省略了已分配块的脚部，并将 PREV 和 SUCC 指针的大小压缩为 4 bytes
 *
 * 空闲块的结构如下：
 * ——————————————————————————————————————
 * ｜ SIZE() ｜ 0 prev_alloc curr_alloc |(4 bytes) HEADER
 * ——————————————————————————————————————
 * |               PREV                |(4 bytes)
 * ——————————————————————————————————————
 * |               SUCC                |(4 bytes)
 * ——————————————————————————————————————
 * |           SOMETHING IN IT         |
 * ——————————————————————————————————————
 * |          PADDING(OPTIONAL)        |
 * ______________________________________
 * ｜ SIZE() ｜ 0 prev_alloc curr_alloc |(4 bytes) FOOTER
 * ——————————————————————————————————————
 *
 * 已分配块的结构如下：
 * ——————————————————————————————————————
 * ｜ SIZE() ｜ 0 prev_alloc curr_alloc |(4 bytes) HEADER
 * ——————————————————————————————————————
 * ——————————————————————————————————————
 * |           SOMETHING IN IT         |
 * ——————————————————————————————————————
 * |          PADDING(OPTIONAL)        |
 * ______________________________________
 *
 * 分离适配空闲链表的具体组织方式见函数
 * delete_free, insert_free, get_class_index
 *
 * 空闲链表的操作采用 LIFO 逻辑，并使用了有限制的最佳适配策略来选取合适的空闲块
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
// #define DEBUG
// #define REALLOC_CAREFULL

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
#define NULL_PTR mm_heap_lo /* 在压指针时可当作空指针的替代使用 */
#define CLASS_NUM 20        /* 分离适配的组数 */

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT - 1)) & ~0x7)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* 包装块的头部和脚部，包含前一个块和当前块的分配信息以及当前块的大小 */
#define PACK(size, alloc_pred, alloc_curr) ((size) | \
                                            (alloc_pred << 1) | (alloc_curr))

/* 将指针的真实值与堆的初始地址相减，可以将指针压缩至 4 bytes */
/* 获取真实的指针值 */
#define TRUEP(p) ((void *)(((unsigned long)(p)) + \
                           ((unsigned long)(mm_heap_lo))))

/* 压缩指针大小 */
#define ZIPP(p) ((unsigned int)(((unsigned long)(p)) - \
                                ((unsigned long)(mm_heap_lo))))

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

/* 操作空闲链表组的头指针,因为要做8字节对齐，所以不在对这些指针做压缩处理 */
#define SET_ROOT(i, ptr) (*(unsigned long *)(heap_classp + i * DSIZE) = \
                              (unsigned long)ptr)
#define GET_ROOT_PTR(i) ((void *)(*(unsigned long *)(heap_classp + i * DSIZE)))
#define GET_ROOT_ADD(i) ((void *)(heap_classp + i * DSIZE))

static char *heap_listp = NULL;  /* Pointer to first block */
static char *heap_tailp = NULL;  /* 指向堆的结尾 */
static char *heap_classp = NULL; /* 指向分离链表头指针组的开始 */
static void *mm_heap_lo;         /* 堆的初始地址，避免过多调用 mem_heap_lo 函数  */

static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *ptr);
static void delete_free(void *ptr);
static void instert_free(void *ptr);
static int get_class_index(size_t size);
/*
 *  初始化堆的结构
 *  分离适配堆的初始结构为，
 *
 *  分离链表的基指针组 : DSIZE*CLASS_NUM
 *  1个对齐块：4 字节 0/1/1，
 *  序言块，8 字节 8/1/1
 *  结尾块：4 字节，0/1/1
 *  最后扩展堆
 */
int mm_init(void)
{
    // 将空指针设置为mem_heap_lo，既可以使指针不在堆的可用区域内，又可以在压缩指针时避免溢出
    mm_heap_lo = mem_heap_lo();
    if ((heap_classp = mem_sbrk(CLASS_NUM * DSIZE + 5 * WSIZE)) == (void *)-1)
        return -1;
    for (int i = 0; i < CLASS_NUM; ++i) // 设基指针组初始值为空
        SET_ROOT(i, NULL_PTR);
    PUT(heap_classp + (CLASS_NUM * DSIZE), PACK(0, 1, 1)); // 对齐块
    heap_listp = heap_classp + (CLASS_NUM * DSIZE) + (DSIZE);
    PUT(HDRP(heap_listp), PACK(8, 1, 1)); // 序言块
    PUT(FTRP(heap_listp), PACK(8, 1, 1));
    heap_tailp = NEXT_BLKP(heap_listp);
    PUT(HDRP(heap_tailp), PACK(0, 1, 1)); // 结尾块
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}
/*
 * malloc 至少需要分配 size 这么大的空间
 *（可能因为对齐的原因会更大一点，8 byte 对齐），不能超出堆的范围，也不能覆盖其他已分配的区域
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
        dbg_printf("fit malloc 0x%lx => %p\n", asize, bp);
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    dbg_printf("extend malloc 0x%lx => %p\n", asize, bp);
    return bp;
}

/*
 * free
 * 释放 ptr 指针指向的区域（这个区域必须是已分配的），free(NULL) 什么都不做
 */
void free(void *ptr)
{
    dbg_printf("mm_free => ptr: 0x%lx\n", (unsigned long)ptr);
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
 * realloc：重新分配，根据传入指针的不同，有不同的表现
 * ptr 为 NULL 时，等同于 malloc(size)
 * size 为 0 时，等同于 free(ptr)，需要返回 NULL
 * ptr 不为 NULL 时，一定是指向一个已分配的空间的，就根据新的 size 的大小进行调整，
 * 并让 ptr 指向新的地址（如果是新地址的话），并且旧的区域应该被释放。
 * 另外需要注意的是，需要把原来 block 的值复制过去
 */
void *realloc(void *ptr, size_t size)
{
    size_t old_size;
    size_t new_size;

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
            void *rest_ptr;
            int prev_alloc;
            prev_alloc = GET_ALLOC_PREV(HDRP(ptr));
            PUT(HDRP(ptr), PACK(new_size, prev_alloc, 1));
            // 处理剩余块
            rest_ptr = NEXT_BLKP(ptr);
            PUT(HDRP(rest_ptr), PACK(old_size - new_size, 1, 1));
            free(rest_ptr);
            new_ptr = ptr;
        }
        else
            new_ptr = ptr;
    }
    else
    {
//若新分配的块大小大于旧的块大小
#ifdef REALLOC_CAREFULL
        void *next_ptr;
        void *rest_ptr;
        size_t next_size;
        int next_alloc, prev_alloc;
        next_ptr = NEXT_BLKP(ptr);
        next_size = GET_SIZE(HDRP(next_ptr));
        next_alloc = GET_ALLOC_CURR(HDRP(next_ptr));
        prev_alloc = GET_ALLOC_PREV(HDRP(ptr));
        if (next_alloc == 0 && old_size + next_size >= new_size)
        {

            if (old_size + next_size - new_size >= (4 * WSIZE))
            {
                delete_free(next_ptr);
                PUT(HDRP(ptr), PACK(new_size, prev_alloc, 1));
                rest_ptr = NEXT_BLKP(ptr);
                size_t rest_size = old_size + next_size - new_size;
                PUT(HDRP(rest_ptr), PACK(rest_size, 1, 0));
                PUT(FTRP(rest_ptr), PACK(rest_size, 1, 0));
                instert_free(rest_ptr);
            }
            else
            {
                // 处理下下块的 prev_alloc 位
                void *nn_ptr;
                size_t nn_size;
                int nn_curr_alloc;
                nn_ptr = NEXT_BLKP(next_ptr);
                nn_curr_alloc = GET_ALLOC_CURR(HDRP(nn_ptr));
                nn_size = GET_SIZE(HDRP(nn_ptr));
                PUT(HDRP(nn_ptr), PACK(nn_size, 1, nn_curr_alloc));
                if (nn_curr_alloc == 0)
                    PUT(FTRP(nn_ptr), PACK(nn_size, 1, nn_curr_alloc));
                delete_free(next_ptr);
                PUT(HDRP(ptr), PACK(old_size + next_size, prev_alloc, 1));
            }
            new_ptr = ptr;
        }
        else
        {
            new_ptr = malloc(new_size);
            memcpy(new_ptr, ptr, new_size);
            free(ptr);
        }
#else
        new_ptr = malloc(new_size);
        memcpy(new_ptr, ptr, new_size);
        free(ptr);
#endif
    }
    dbg_printf("realloc %lu => %p\n", new_size, new_ptr);
    return new_ptr;
}

/*
 * calloc - 分配一个有 nmemb 个大小为 size 的数组，并将之初始化
 */
void *calloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
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

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        dbg_printf("extend error %lu\n", size);
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
    // 将新扩展的块放在链表头部
    instert_free(p);
    PUT(HDRP(heap_tailp), PACK(0, 0, 1)); // 结尾块的头部

    /* Coalesce if the previous block was free */
    return coalesce(p);
}
/* coalesce 合并空闲块 */
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
    instert_free(res_ptr);
    return res_ptr;
}
/* find_fit 寻找合适的块的位置，使用有最高搜索次数限制的最佳适配 */
static void *find_fit(size_t asize)
{
    void *bp;
    void *minp = NULL;
    int cnt = 0, index = get_class_index(asize);
    size_t min_dsize = ~0, tmp_size, tmp_dsize;
    int flag = 0;
    for (; index < CLASS_NUM; ++index)
    {
        bp = GET_ROOT_PTR(index);
        if (bp == NULL_PTR)
            continue;
        cnt = 0;
        for (; bp != NULL_PTR; bp = GET_SUCC(bp))
        {
            tmp_size = GET_SIZE(HDRP(bp));

            if (tmp_size < asize)
                continue;
            flag = 1;
            cnt++;
            tmp_dsize = tmp_size - asize;
            if (tmp_dsize < min_dsize)
            {
                min_dsize = tmp_dsize;
                minp = bp;
            }
            if (cnt == 8)
                return minp;
        }
        if (flag == 1)
            return minp;
    }
    return minp;
}

/* place 放置某个块，做空闲块的分割 */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    void *next_ptr = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC_PREV(HDRP(bp));
    // 空闲块最小为 16bytes，若分割后剩余部分大于 16 bytes，则分配空闲块，否则，把剩余部分当作填充物
    if (csize - asize >= 4 * WSIZE)
    {
        delete_free(bp);
        PUT(HDRP(bp), PACK(asize, prev_alloc, 1)); // 只分配头部即可
        void *rest_ptr = NEXT_BLKP(bp);            // 处理剩余块
        PUT(HDRP(rest_ptr), PACK(csize - asize, 1, 0));
        PUT(FTRP(rest_ptr), PACK(csize - asize, 1, 0));
        instert_free(rest_ptr);
    }
    else
    {
        // 将空闲块从链表中删除,并为当前块设置头部
        delete_free(bp);
        PUT(HDRP(bp), PACK(csize, prev_alloc, 1));
        // 设置下一块的头部和脚部
        size_t next_size = GET_SIZE(HDRP(next_ptr));
        size_t next_alloc = GET_ALLOC_CURR(HDRP(next_ptr));
        PUT(HDRP(next_ptr), PACK(next_size, 1, next_alloc));
        if (next_alloc == 0)
            PUT(FTRP(next_ptr), PACK(next_size, 1, next_alloc));
    }
}
/* 使用 LIFO 策略 */
/* delete_free 在空闲链表中删除某个块 */
static void delete_free(void *ptr)
{
    void *pred_ptr = GET_PRED(ptr);
    void *succ_ptr = GET_SUCC(ptr);
    if (succ_ptr != NULL_PTR)
        SET_PRED(succ_ptr, pred_ptr);

    if ((unsigned long)pred_ptr > (unsigned long)heap_listp) // 若上个空闲块不是基指针
        SET_SUCC(pred_ptr, succ_ptr);
    else
        *(unsigned long *)pred_ptr = (unsigned long)succ_ptr;
}
/* instert_free 在空闲链表中插入某个块 */
static void instert_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    int class_index = get_class_index(size);
    void *succ_ptr = GET_ROOT_PTR(class_index);
    if (succ_ptr != NULL_PTR)
        SET_PRED(succ_ptr, ptr);
    SET_SUCC(ptr, succ_ptr);
    SET_PRED(ptr, GET_ROOT_ADD(class_index));
    SET_ROOT(class_index, ptr);
}
/*
 * get_class_index 获取此大小块应在的空闲链表组：
 *  共分为 CLASS_NUM 个组，其范围分别是：
 *  {1<<4},{1<<4+1 ~ 1<<5},{1<<5+1 ~ 1<<6},....,{1<<(CLASS_NUM+2)+1 ~ +INF}
 *  使用线性查找获得其所在组别
 */
static int get_class_index(size_t size)
{
    if (size <= 16)
        return 0;
    int index = 0;
    size_t class_max_size;
    for (; index < CLASS_NUM - 1; ++index)
    {
        class_max_size = (1 << (4 + index));
        if (size <= class_max_size)
            return index;
    }
    return index;
}

/*
 * mm_checkheap
 * Checking the heap (implicit list, explicit list, segregated list):
 * – Check epilogue and prologue blocks.
 * – Check each block’s address alignment.
 * – Check heap boundaries.
 * – Check each block’s header and footer:
 * size (minimum size, alignment), previous/next allocate/free bit consistency,
 * header and footer matching each other.
 * – Check coalescing: no two consecutive free blocks in the heap.
 * • Checking the free list (explicit list, segregated list):
 * – All next/previous pointers are consistent
 * (if A’s next pointer points to B, B’s previous pointer
 * should point to A).
 * – All free list pointers points
 * between mem heap lo() and mem heap high().
 * – Count free blocks by iterating through every block
 * and traversing free list by pointers and see if they match.
 * – All blocks in each list bucket
 * fall within bucket size range (segregated list).
 */
void mm_checkheap(int lineno)
{
#ifdef DEBUG
    if (!lineno)
        return;
    lineno = lineno;
    char *hp = heap_listp;
    char *tp = heap_tailp;
    /* 是否在报错时退出 */
    int error_exit = 0;
    /* 检查堆是否初始化 */
    dbg_printf("\n--------------\nHEAP CHECK BEGIN\n--------------\n");
    if (hp == NULL)
        dbg_printf("Heap Uninitialized\n\n");
    if (lineno & 0x1)
        error_exit = 1;
    /* 检查序言块和结尾块 */
    if (lineno & 0x2)
    {
        dbg_printf(
            "Prilogue Header "
            "addr = 0x%lx , size = %u , curr_alloc = %u , prev_alloc = %u\n",
            (unsigned long)hp,
            GET_SIZE(HDRP(hp)),
            GET_ALLOC_CURR(HDRP(hp)),
            GET_ALLOC_PREV(HDRP(hp)));
        dbg_printf(
            "Prilogue Footer "
            "addr = 0x%lx , size = %u , curr_alloc = %u ,prev_alloc = %u\n",
            (unsigned long)hp,
            GET_SIZE(FTRP(hp)),
            GET_ALLOC_CURR(FTRP(hp)),
            GET_ALLOC_PREV(FTRP(hp)));
        dbg_printf(
            "Epilogue Header "
            "addr = 0x%lx , size = %u , curr_alloc = %u ,prev_alloc = %u\n\n",
            (unsigned long)tp,
            GET_SIZE(HDRP(tp)),
            GET_ALLOC_CURR(HDRP(tp)),
            GET_ALLOC_PREV(HDRP(tp)));
    }
    /* 检查堆的总体情况 */
    if (lineno & 0x4)
    {
        dbg_printf("Heap Information:\n");
        dbg_printf(
            "Heap size = %ld\n"
            "Heap first address = 0x%lx\n"
            "Heap last address = 0x%lx\n\n",
            mem_heapsize(),
            (unsigned long)mem_heap_lo(),
            (unsigned long)mem_heap_hi());
    }
    /* 检查每一个块的具体情况 */
    if ((lineno & 0x8) || (lineno & 0x10) || (lineno & 0x20))
    {
        dbg_printf(
            "\n--------------\n"
            "CHECK BLOCK"
            "\n--------------\n");
        int index = 1; // 每个块的序号
        int head_curr_alloc;
        int head_size;
        int head_prev_alloc;
        int foot_curr_alloc;
        int foot_size;
        int foot_prev_alloc;
        int last_alloc;
        void *bp = heap_listp;
        void *headp;
        void *footp;
        void *lastp;
        void *nextp;

        while (1)
        {
            int error = 0;
            lastp = bp;
            bp = NEXT_BLKP(bp);
            headp = HDRP(bp);
            head_curr_alloc = GET_ALLOC_CURR(headp);
            head_size = GET_SIZE(headp);
            head_prev_alloc = GET_ALLOC_PREV(headp);
            // 检查块内的具体情况
            if (lineno & 0x8)
            {
                // 检查地址是否对齐
                if (((unsigned long)bp) % 8 != 0)
                {
                    dbg_printf(
                        "ADDRESS UNALIGNED! "
                        "index: %d;"
                        "addr: 0x%lx;\n",
                        index,
                        (unsigned long)bp);
                    error = error | 0x1;
                }

                // 检查地址是否在堆的范围内
                if (!in_heap(bp))
                {
                    dbg_printf(
                        "ADDRESS CROSS THE BORDER! "
                        "index: %d;"
                        "addr: 0x%lx;\n",
                        index,
                        (unsigned long)bp);
                    error = error | 0x2;
                }

                last_alloc = GET_ALLOC_CURR(HDRP(lastp));
                // 检查当前块的 prev_alloc 位和上个块真实的 alloc 位是否匹配
                if (last_alloc != head_prev_alloc)
                {
                    dbg_printf(
                        "TRUE LAST AND PREV ALLOC MISMATCH! "
                        "index: %d;"
                        "addr: 0x%lx;"
                        "last_alloc: %d;"
                        "prev_alloc:%d\n",
                        index,
                        (unsigned long)bp,
                        last_alloc,
                        head_prev_alloc);
                    error = error | 0x4;
                }
                if (head_size == 0)
                    break;
                // 如果当前块空闲，检查其头部和脚部是否匹配
                if (head_curr_alloc == 0)
                {
                    footp = FTRP(bp);
                    foot_curr_alloc = GET_ALLOC_CURR(footp);
                    foot_size = GET_SIZE(footp);
                    foot_prev_alloc = GET_ALLOC_PREV(footp);
                    if (head_curr_alloc != foot_curr_alloc)
                    {
                        dbg_printf(
                            "SIZE MISMATCH! "
                            "index: %d;"
                            "addr: 0x%lx;"
                            "head_size: %d;"
                            "foot_size:%d\n",
                            index,
                            (unsigned long)bp,
                            head_size,
                            foot_size);
                        error = error | 0x8;
                    }
                    if (head_prev_alloc != foot_prev_alloc)
                    {
                        dbg_printf(
                            "PREV ALLOC MISMATCH! "
                            "index: %d;"
                            "addr: 0x%lx;"
                            "head_prev_alloc: %d;"
                            "foot_prev_alloc:%d\n",
                            index,
                            (unsigned long)bp,
                            head_prev_alloc,
                            foot_prev_alloc);
                        error = error | 0x8;
                    }
                    if (head_curr_alloc != foot_curr_alloc)
                    {
                        dbg_printf(
                            "CURR ALLOC MISMATCH! "
                            "index: %d;"
                            "addr: 0x%lx;"
                            "head_curr_alloc: %d;"
                            "foot_curr_alloc:%d\n",
                            index,
                            (unsigned long)bp,
                            head_curr_alloc,
                            foot_curr_alloc);
                        error = error | 0x8;
                    }
                }
            }
            if (head_size == 0)
                break;
            // 检查是否有连续的空闲块
            if (lineno & 0x10)
            {
                nextp = NEXT_BLKP(bp);
                if ((head_curr_alloc == 0) && 
                (GET_ALLOC_CURR(HDRP(nextp)) == 0))
                {
                    dbg_printf(
                        "NOT COALESCED! "
                        "Block (index: %d, addr: 0x%lx) "
                        "and block (index: %d, addr: 0x%lx)",
                        index, (unsigned long)bp,
                        index + 1, (unsigned long)nextp);
                    error = error | 0x10;
                }
            }
            // 打印每个块的具体信息
            if (lineno & 0x20)
            {
                dbg_printf(
                    "index: %d;"
                    "addr: 0x%lx; "
                    "size: %d; "
                    "curr_alloc: %d; "
                    "prev_alloc:%d\n",
                    index,
                    (unsigned long)bp,
                    head_size,
                    head_curr_alloc,
                    head_prev_alloc);
            }
            if (error && error_exit)
            {
                dbg_printf(
                    "\n--------------\n"
                    "BLOCK CHECK FINISH"
                    "\n--------------\n");
                exit(error);
            }
            index++;
        }
    }
    if (lineno & 0x80 || lineno & 0x40)
    {
        void *class_addr;
        void *root_blk_addr;
        void *bp;
        void *pred_ptr;
        int cnt;
        dbg_printf(
            "\n--------------\n"
            "CHECK FREE LIST"
            "\n--------------\n");
        for (int i = 0; i < CLASS_NUM; ++i)
        {
            class_addr = GET_ROOT_ADD(i);
            root_blk_addr = GET_ROOT_PTR(i);
            dbg_printf(
                "CLASS %d:\n\t"
                "class_addr: 0x%lx; "
                "root_block_addr: 0x%lx ;"
                "class_size: %d\n",
                i,
                (unsigned long)class_addr,
                (unsigned long)root_blk_addr,
                1 << (i + 4));
            pred_ptr = class_addr;
            bp = root_blk_addr;
            cnt = 0;
            while (1)
            {
                if (bp == NULL_PTR)
                    break;
                cnt++;
                if (lineno & 0x40)
                {
                    int error = 0;
                    // 判断指针是否越界
                    if (!in_heap(bp))
                    {
                        dbg_printf(
                            "\tPOINTER CROSS THE BORDER! "
                            "id: %d; "
                            "ptr: 0x%lx; "
                            "mm_heap_lo: 0x%lx;"
                            "mm_heap_hi: 0x%lx\n",
                            cnt,
                            (unsigned long)bp,
                            (unsigned long)mm_heap_lo,
                            (unsigned long)mem_heap_hi);
                        error |= 0x20;
                    }
                    // 判断指针是否匹配
                    if (GET_PRED(bp) != pred_ptr)
                    {
                        dbg_printf(
                            "\tPOINTER MISMATCH! "
                            "id: %d; "
                            "ptr: 0x%lx; "
                            "pred_ptr: 0x%lx\n",
                            cnt,
                            (unsigned long)bp,
                            (unsigned long)pred_ptr);
                        error |= 0x40;
                    }

                    // 判断空闲块的大小是否适配该组
                    int class_index = get_class_index(GET_SIZE(HDRP(bp)));
                    if (i != class_index)
                    {
                        dbg_printf(
                            "\tBLOCK SIZE MISMATCH CLASS SIZE! "
                            "id: %d; "
                            "blk_size: %d; "
                            "class_size: %d; "
                            "class_index: %d\n",
                            cnt,
                            GET_SIZE(HDRP(bp)),
                            1 << (4 + i),
                            class_index);
                        error |= 0x80;
                    }
                    if (error && error_exit)
                        exit(error);
                }
                if (lineno & 0x80)
                {
                    dbg_printf(
                        "\tINFO! "
                        "id: %d; "
                        "addr: 0x%lx; "
                        "pred: 0x%lx; "
                        "succ: 0x%lx; "
                        "size: %d\n",
                        cnt,
                        (unsigned long)bp,
                        (unsigned long)GET_PRED(bp),
                        (unsigned long)GET_SUCC(bp),
                        GET_SIZE(HDRP(bp)));
                }
                pred_ptr = bp;
                bp = GET_SUCC(bp);
            }
            dbg_printf(
                "\tclass %d "
                "has %d blocks\n",
                i,
                cnt);
        }
        dbg_printf(
            "\n--------------\n"
            "FREE LIST CHECK FINISH"
            "\n--------------\n");
    }
    dbg_printf(
        "\n--------------\n"
        "HEAP CHECK FINISH"
        "\n--------------\n");
#endif
    return;
}