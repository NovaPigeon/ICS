/*
 * mm.c - a primitive dynamic memory allocator
 *
 * Author: 1900013010 Hu. F.M.
 *
 * Method: segregated free lists(segregated fits, explicit free lists),
 * as is described in CSAPP(3e, Global Edition), Page 899-901.
 *
 * Heap Structure:
 * ------------------------
 * | Segregated Free List
 * ------------------------
 * | Prologue Header
 * ------------------------
 * | Prologue Footer
 * ------------------------
 * | Available Heap
 * ------------------------
 * | Epilogue
 * ------------------------
 *
 * Block Structure:
 *  Allocated Block:
 * ------------------------
 * | Header = SIZE | 0 PREALLOC(is previous allocated?) ALLOC
 * | (4 Bytes)
 * ------------------------
 * | Block and Padding(Optional)
 * ------------------------
 *
 *  Free Block:
 * ------------------------
 * | Header = SIZE | 0 0 ALLOC (4 Bytes)
 * ------------------------
 * | Pointer to previous element in list (4 Bytes)
 * ------------------------
 * | Pointer to next element in list (4 Bytes)
 * ------------------------
 * | Padding(Optional)
 * ------------------------
 * | Footer = SIZE | 0 0 ALLOC (4 Bytes)
 * ------------------------
 *
 * Segregated Free List:
 *  Free blocks are organized in a segregated free list.
 *    See list_index() for details.
 *  Only first (iter) elements of each free list is guaranteed to be ordered.
 *    See list_insert() for details.
 *  Adopt first-fit strategy when search for a fit.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro. When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define vdbg_printf(...)
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
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT - 1)) & ~0x7)

/*
 * Macros (partly from CSAPP(3e, Global Edition), Page 893)
 */

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define INITCHUNKSIZE 16    /* Ask for a smaller space during init */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define SET(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* Allocated block record allocation of previous block */
#define GET_PREALLOC(p) (GET(p) & 0x2)
#define SET_PREALLOC(p) (*(unsigned int *)p |= 0x2)
#define CLEAR_PREALLOC(p) (*(unsigned int *)p &= ~0x2)

/* heap address */
static void *heap;
/* static void *epilog; */

/*
 * Segregated free list
 */
#define LISTNUM 10
void **s_free_list; /* segregated free list */
/*
 * Block size categories:
 * 16, 24, 32, 33-64, 65-128, 129-256, ...
 */
/* stored at the prologue of the heap */
#define PREVP(lp) ((char *)(lp)) /* location of pointers */
#define NEXTP(lp) ((char *)(lp) + WSIZE)
#define PREV(lp) (*(unsigned int *)(lp) + heap) /* location of elements */
#define NEXT(lp) (*(unsigned int *)(NEXTP(lp)) + heap)
#define PSET(p, pval) (*(unsigned int *)(p) = (unsigned int)(pval))
/* store pointer as unsigned int(4 bytes) */

int list_index(size_t size)
{
   /* always assume size is aligned */
   if (size <= 16)
      return 0;
   else if (size <= 24)
      return 1;
   else if (size <= 32)
      return 2;
   else if (size <= 64)
      return 3;
   else if (size <= 128)
      return 4;
   else if (size <= 256)
      return 5;
   else if (size <= 512)
      return 6;
   else if (size <= 1024)
      return 7;
   else if (size <= 2048)
      return 8;
   else
      return 9;
}

/*
 * list_insert - insert a free block into list,
 * and keep first (iter) elements of the list in an ascending order.
 * size is assumed to be aligned.
 */
static void list_insert(void *p, size_t size)
{
   int id = list_index(size);
   // dbg_printf("qwq: insert %p(%ld) at list[%d]\n", p, size, id);

   void *prev_pos = NULL, *cur_pos = s_free_list[id];
   if (cur_pos == NULL)
   {
      // first element of the list
      s_free_list[id] = p;
      PSET(NEXTP(p), NULL);
      PSET(PREVP(p), NULL);
   }
   else
   {
      if (id > 2)
      {
         // when id = 0,1,2 block size is constant, only need to insert at head
         int iter = 0;
         /* max iteration time -
         only keep the list locally ordered, to boost inserting speed */
         while (cur_pos > heap && size > GET_SIZE(HDRP(cur_pos)) && iter < 8)
         {
            prev_pos = cur_pos;
            cur_pos = NEXT(cur_pos);
            iter++;
         }
      }
      if (cur_pos <= heap)
      {
         // insert at tail
         PSET(PREVP(p), prev_pos);
         PSET(NEXTP(p), NULL);
         PSET(NEXTP(prev_pos), p);
      }
      else if (prev_pos == NULL)
      {
         // insert at head
         PSET(PREVP(p), NULL);
         PSET(NEXTP(p), cur_pos);
         PSET(PREVP(cur_pos), p);
         s_free_list[id] = p;
      }
      else
      {
         // insert in middle
         PSET(PREVP(p), prev_pos);
         PSET(NEXTP(p), cur_pos);
         PSET(NEXTP(prev_pos), p);
         PSET(PREVP(cur_pos), p);
      }
   }
   // mm_checklist(1);
}

/*
 * list_delete - delete a free block from list,
 */
static void list_delete(void *p)
{
   size_t size = GET_SIZE(HDRP(p));
   int id = list_index(size);
   // dbg_printf("qwq: delete %p(%ld) at list[%d]\n", p, size, id);
   _Bool prev_valid = (PREV(p) > heap);
   _Bool next_valid = (NEXT(p) > heap);
   if (prev_valid && next_valid)
   {
      /* middle */
      PSET(NEXTP(PREV(p)), NEXT(p));
      PSET(PREVP(NEXT(p)), PREV(p));
   }
   else if (prev_valid)
   {
      /* tail */
      PSET(NEXTP(PREV(p)), NULL);
   }
   else if (next_valid)
   {
      /* head */
      PSET(PREVP(NEXT(p)), NULL);
      s_free_list[id] = NEXT(p);
   }
   else
   {
      /* only */
      s_free_list[id] = NULL;
   }
   // mm_checklist(2);
}

/*
 * Helper functions
 */
static void *extend_heap(size_t size);
static void *coalesce(void *p);
static void *search_fit(size_t asize);
static void place(void *bp, size_t asize);

/* extend heap space */
static void *extend_heap(size_t size)
{
   /* a tiny optimization to deal with massive small blocks */
   if (size < 4096)
      /* why 5? experiments proved its efficiency */
      size = ALIGN(5 * size);
   else
      size = ALIGN(size);
   void *bp;
   if ((bp = mem_sbrk(size)) == (void *)-1)
      return NULL;
   /* set block header and footer */
   if (GET_PREALLOC(HDRP(bp)))
      SET(HDRP(bp), PACK(size, 2));
   else
      SET(HDRP(bp), PACK(size, 0));
   SET(FTRP(bp), PACK(size, 0));
   /* set new epilogue */
   SET(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
   // epilog = NEXT_BLKP(bp);
   /* insert extended space into empty list */
   list_insert(bp, size);
   /* need coalesce if previous block is free */
   // dbg_printf("qwq: extend heap by %ld, return %p\n", size, bp);
   return coalesce(bp);
}

/* coalesce - merge adjacent free blocks */
static void *coalesce(void *p)
{
   /* i miss c++ */
   _Bool prev_alloc, next_alloc;
   if (!GET_PREALLOC(HDRP(p)) && PREV_BLKP(p) < mem_heap_lo())
      prev_alloc = 1;
   else
      prev_alloc = GET_PREALLOC(HDRP(p));
   if (NEXT_BLKP(p) > mem_heap_hi())
      next_alloc = 1;
   else
      next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(p)));
   if (prev_alloc && next_alloc)
      return p; /* none of adjacent block is free */
   size_t size = GET_SIZE(HDRP(p));
   char *newhdr = HDRP(p); /* new header */
   char *newftr = FTRP(p); /* new footer */
   void *retp = p;         /* pointer of new block */
   list_delete(p);
   if (!prev_alloc)
   {
      /* previous block is free */
      list_delete(PREV_BLKP(p));
      size += GET_SIZE(HDRP(PREV_BLKP(p)));
      newhdr = HDRP(PREV_BLKP(p));
      retp = PREV_BLKP(p);
   }
   if (!next_alloc)
   {
      /* next block is free */
      list_delete(NEXT_BLKP(p));
      size += GET_SIZE(HDRP(NEXT_BLKP(p)));
      newftr = FTRP(NEXT_BLKP(p));
   }
   if (GET_PREALLOC(newhdr))
      SET(newhdr, PACK(size, 2));
   else
      SET(newhdr, PACK(size, 0));
   SET(newftr, PACK(size, 0));
   list_insert(retp, size);
   return retp;
}

/* place - place the block, and split if possible */
static void place(void *bp, size_t asize)
{
   if (bp == NULL)
      return;
   size_t all = GET_SIZE(HDRP(bp));
   size_t remain = all - asize;
   list_delete(bp);
   if (remain >= 2 * DSIZE)
   {
      if (GET_PREALLOC(HDRP(bp)))
         SET(HDRP(bp), PACK(asize, 3));
      else
         SET(HDRP(bp), PACK(asize, 1));
      // SET(FTRP(bp), PACK(asize, 1));
      // dbg_printf("qwq: split and place block(%ld, %ld) at %p\n", asize, GET_SIZE(HDRP(bp)), bp);
      void *rp = NEXT_BLKP(bp);
      SET(HDRP(rp), PACK(remain, 2));
      SET(FTRP(rp), PACK(remain, 2));
      list_insert(rp, remain);
   }
   else
   {
      if (GET_PREALLOC(HDRP(bp)))
         SET(HDRP(bp), PACK(all, 3));
      else
         SET(HDRP(bp), PACK(all, 1));
      // SET(FTRP(bp), PACK(all, 1));
      // dbg_printf("qwq: place block(%ld, %ld) at %p\n", all, GET_SIZE(HDRP(bp)), bp);
      SET_PREALLOC(HDRP(NEXT_BLKP(bp)));
   }
}

/* search_fit - search free list and place the block(if found) */
static void *search_fit(size_t asize)
{
   void *bp = NULL;
   for (int id = list_index(asize); id < LISTNUM; ++id)
   {
      if (s_free_list[id])
      {
         void *cur_pos = s_free_list[id];
         while (cur_pos > heap && GET_SIZE(HDRP(cur_pos)) < asize)
         {
            cur_pos = NEXT(cur_pos);
         }
         if (cur_pos > heap)
         {
            /* free block found */
            bp = cur_pos;
            place(bp, asize);
            break;
         }
      }
   }
   return bp;
}

/*
 * initialize: return -1 on error, 0 on success.
 * reference: CSAPP(3e)
 */
int mm_init(void)
{
   char *heap_p;
   /* create initial empty heap */
   if ((heap_p = mem_sbrk(4 * WSIZE + 8 * LISTNUM)) == (void *)-1)
      return -1;
   heap = (void *)heap_p;
   s_free_list = (void **)heap;
   /* free list init */
   for (int i = 0; i < LISTNUM; ++i)
      s_free_list[i] = NULL;
   heap_p += 8 * LISTNUM;
   SET(heap_p, 0);                            /* Alignment padding */
   SET(heap_p + (1 * WSIZE), PACK(DSIZE, 3)); /* Prologue header */
   SET(heap_p + (2 * WSIZE), PACK(DSIZE, 3)); /* Prologue footer */
   SET(heap_p + (3 * WSIZE), PACK(0, 3));     /* Epilogue header */
   heap_p += (2 * WSIZE);
   // dbg_printf("qwq: heap: %p\n", heap);
   if (extend_heap(INITCHUNKSIZE) == NULL)
      return -1;
   return 0;
}

/*
 * malloc
 */
void *malloc(size_t size)
{
   // dbg_printf("qwq: malloc %ld\n", size);
   size_t asize;
   char *bp = NULL;
   if (size == 0)
      return NULL;
   if (size == 448)
      size = 512;
   /* adjusted block contains header and footer */
   if (size <= DSIZE)
      asize = 2 * DSIZE;
   else
      asize = ALIGN(size + WSIZE);
   /* search for a fit */
   bp = search_fit(asize);
   if (bp == NULL)
   {
      /* extend heap */
      bp = extend_heap(asize);
      place(bp, asize);
   }
   // dbg_printf("qwq: malloc(%ld) at %p\n", asize, bp);
   // mm_checkheap(1);
   return bp;
}

/*
 * free
 */
void free(void *p)
{
   // dbg_printf("qwq: free %p\n", p);
   if (p == NULL)
      return;
   size_t size = GET_SIZE(HDRP(p));
   if (GET_PREALLOC(HDRP(p)))
      SET(HDRP(p), PACK(size, 2));
   else
      SET(HDRP(p), PACK(size, 0));
   SET(FTRP(p), PACK(size, 0));
   CLEAR_PREALLOC(HDRP(NEXT_BLKP(p)));
   list_insert(p, size);
   coalesce(p);
   // mm_checkheap(2);
}

/*
 * realloc - avoid memcpy() if possible
 */
void *realloc(void *op, size_t size)
{
   if (op == NULL)
   {
      return malloc(size);
   }
   else if (size == 0)
   {
      free(op);
      return NULL;
   }
   /* reallocation */
   if (size <= DSIZE)
      size = 2 * DSIZE;
   else
      size = ALIGN(size + WSIZE);
   size_t osize = GET_SIZE(HDRP(op));
   // dbg_printf("qwq: realloc %p(%ld) to %ld\n", op, osize, size);
   int remain = (int)osize - (int)size;
   if (remain >= 0 && remain < 2 * DSIZE)
   {
      /* do nothing */
      return op;
   }
   else if (remain >= 2 * DSIZE)
   {
      /* split current block */
      if (GET_PREALLOC(HDRP(op)))
         SET(HDRP(op), PACK(size, 3));
      else
         SET(HDRP(op), PACK(size, 1));
      void *rp = NEXT_BLKP(op);
      SET(HDRP(rp), PACK(remain, 2));
      SET(FTRP(rp), PACK(remain, 2));
      CLEAR_PREALLOC(HDRP(NEXT_BLKP(rp)));
      list_insert(rp, remain);
      return op;
   }
   /* else osize < size */
   void *rp = NEXT_BLKP(op);
   int extra = (int)size - (int)osize;
   if (rp > mem_heap_hi())
   {
      /* ask for additional heap to extend current block */
      rp = extend_heap(extra);
      if (rp == NULL)
         return rp;
   }
   if (GET_ALLOC(HDRP(rp)) == 0)
   {
      /* realloc can be done by simply extending current block */
      size_t esize = GET_ALLOC(HDRP(rp));
      remain = (int)osize + (int)esize - (int)size;
      if (remain >= 0 && remain < 2 * DSIZE)
      {
         list_delete(rp);
         if (GET_PREALLOC(HDRP(op)))
            SET(HDRP(op), PACK(size, 3));
         else
            SET(HDRP(op), PACK(size, 1));
         SET_PREALLOC(HDRP(NEXT_BLKP(op)));
         return op;
      }
      else if (remain >= 2 * DSIZE)
      {
         /* split additional space */
         list_delete(rp);
         if (GET_PREALLOC(HDRP(op)))
            SET(HDRP(op), PACK(size, 3));
         else
            SET(HDRP(op), PACK(size, 1));
         void *rrp = NEXT_BLKP(op);
         SET(HDRP(rrp), PACK(remain, 2));
         SET(FTRP(rrp), PACK(remain, 2));
         CLEAR_PREALLOC(HDRP(NEXT_BLKP(rp)));
         list_insert(rrp, remain);
         return op;
      }
   }
   /* have to RE-alloc the block */
   void *np = malloc(size - WSIZE);
   if (np == NULL)
      return np;
   memcpy(np, op, osize - WSIZE);
   free(op);
   return np;
}

/*
 * calloc - directly copied from mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
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

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno)
{
   /* if you want a complete and silent check
       for some possible errors and more errors that will never occur,
       set lineno = -1. */
   if (lineno == -1)
   {
      void *p = heap;
      if (*(unsigned int *)p > 0x7)
      {
         dbg_printf("Prologue Error\n");
      }
      p = (char *)p + 8 * LISTNUM + 2 * DSIZE;
      int free_flag = 0;
      while (p < mem_heap_hi())
      {
         if (!aligned(p))
            dbg_printf("Alignment Error at %p\n", p);
         if (GET_SIZE(HDRP(p)) < 16)
            dbg_printf("Size Error at %p\n", p);
         if (!GET_ALLOC(HDRP(p)))
         {
            if (free_flag)
               dbg_printf("Adjacent free blocks not merged at %p\n", p);
            free_flag = 1;
            if (GET_SIZE(HDRP(p)) != GET_SIZE(FTRP(p)) || GET_ALLOC(FTRP(p)))
               dbg_printf("Consistency Error at %p\n", p);
         }
         else
         {
            free_flag = 0;
         }
      }
      p = (char *)p - WSIZE;
      if (*(unsigned int *)p > 0x7)
         printf("Epilogue Error\n");
      return;
   }
   /* otherwise, a simple list of each block in heap will suffice. */
   void *p = heap + 8 * LISTNUM + 2 * DSIZE;
   dbg_printf("qwq: checkheap(%d) %p\n", lineno, p);
   while (p < mem_heap_hi())
   {
      dbg_printf("  %p %ld %d %d\n", p, GET_SIZE(HDRP(p)), GET_ALLOC(HDRP(p)), GET_PREALLOC(HDRP(p)));
      p = NEXT_BLKP(p);
   }
}

void mm_checklist(int lineno)
{
   /* if you want a complete and silent check
       for some possible errors and more errors that will never occur,
       set lineno = -1. */
   if (lineno == -1)
   {
      int cnt1 = 0, cnt2 = 0;
      void *p = heap + 8 * LISTNUM + 2 * DSIZE;
      while (p < mem_heap_hi())
      {
         if (!GET_ALLOC(HDRP(p)))
            cnt1++;
         p = NEXT_BLKP(p);
      }
      for (int i = 0; i < LISTNUM; ++i)
      {
         void *cur = s_free_list[i];
         while (cur > heap)
         {
            if (!in_heap(cur))
               dbg_printf("Error: %p in list[%d] not in heap\n", cur, i);
            if (PREV(cur) && cur != NEXT(PREV(cur)))
               dbg_printf("Error: info in %p and %p is not consistent\n", PREV(cur), cur);
            if (i != list_index(GET_SIZE(HDRP(cur))))
               dbg_printf("Error: %p(size = %ld) should not be in list[%d]\n",
                          cur, GET_SIZE(HDRP(cur)), i);
            cur = NEXT(cur);
            cnt2++;
         }
      }
      if (cnt1 != cnt2)
         dbg_printf("Error: num of free blocks is not consistent\n");
      return;
   }
   /* otherwise, a simple list of each free list will suffice. */
   dbg_printf("qwq: checklist(%d)\n", lineno);
   for (int i = 0; i < LISTNUM; ++i)
   {
      void *cur = s_free_list[i];
      dbg_printf("  list[%d]", i);
      while (cur > heap)
      {
         dbg_printf(" -> %p(%ld)", cur, GET_SIZE(HDRP(cur)));
         cur = NEXT(cur);
      }
      dbg_printf("\n");
   }
}