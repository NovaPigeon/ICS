#ifndef __CACHE_H__
#define __CACHE_H__

#include"csapp.h"

//#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

/* Recommended max cache and object sizes */
/* CACHE 最大的大小 */
#define MAX_CACHE_SIZE 1049000
/* 每个块最大的大小 */
#define MAX_OBJECT_SIZE 102400
/* 缓存的块总数，约等于 MAX_CACHE_SIZE/MAX_OBJECT_SIZE */
#define MAX_CACHE_BLK 10

/* cache块 */
typedef struct 
{
    /* 内容 */
    char content[MAX_OBJECT_SIZE];
    /* uri，cache 块由 uri 唯一标识 */
    char uri[MAXLINE];
    /* cache 按 LRU 策略进行替换和驱逐 */
    int LRUtag;
    /* content 的大小 */
    int len;
    /* 有效位 */
    int valid;

}cache_t;

/* 读写者锁 */
typedef struct 
{
    sem_t base_lock;
    sem_t write_lock;
    int readcnt;
}rwlock_t;

/* 初始化 cache 和相应的锁*/
void cache_init();
/* 寻找命中的块 */
int cache_search_hit_blk(char* uri);
/* 寻找空闲或应被驱逐的块 */
int cache_search_avilable_blk();
/* 插入新的块 */
void cache_insert_blk(char* uri,char* content,int len);
/* 更新整个 cache 的 LRUtag */
void cache_update_LRU(int index);
/* 检查整个cache */
void cache_debug();

/* 读者优先 */
/* 读者锁 */
void reader_lock(int i);
/* 读者解锁 */
void reader_unlock(int i);
/* 写者锁 */
void writer_lock(int i);
/* 写者解锁 */
void writer_unlock(int i);


#endif