/*
 * cache.c - caching web objects
 */

#include "cache.h"
#include "csapp.h"

cache_t cache[MAX_CACHE_BLK];
rwlock_t rwlock[MAX_CACHE_BLK];

void cache_init()
{
    for (int i = 0; i < MAX_CACHE_BLK; ++i)
    {
        /* 初始化 cache */
        cache[i].valid = 0;
        cache[i].LRUtag = 0;
        cache[i].len = 0;
        strcpy(cache[i].uri, "");

        /* 初始化该 cache 对应的读/写者锁 */
        rwlock[i].readcnt = 0;
        sem_init(&rwlock[i].write_lock, 0, 1);
        sem_init(&rwlock[i].base_lock, 0, 1);
    }
}

int cache_search_hit_blk(char *uri)
{
    for (int i = 0; i < MAX_CACHE_BLK; ++i)
    {
        /* 遍历 cache 对比 uri，若不命中，则返回 -1 */
        reader_lock(i);
        if (cache[i].valid)
        {
            if (strcmp(cache[i].uri, uri) == 0)
            {
                reader_unlock(i);
                return i;
            }
        }
        reader_unlock(i);
    }
    return -1;
}

int cache_search_avilable_blk()
{
    int avilable_blk = 0;
    int maxLRUtag = -1;
    for (int i = 0; i < MAX_CACHE_BLK; ++i)
    {
        /* 遍历 cache，寻找空闲块和 LRUtag 最大的块 */
        reader_lock(i);
        if (cache[i].valid == 0)
        {
            reader_unlock(i);
            return i;
        }
        else
        {
            if (cache[i].LRUtag > maxLRUtag)
            {
                avilable_blk = i;
                maxLRUtag = cache[i].LRUtag;
            }
        }
        reader_unlock(i);
    }
    return avilable_blk;
}

void cache_insert_blk(char *uri, char *content, int len)
{
    int index = cache_search_avilable_blk();
    writer_lock(index);
    strcpy(cache[index].uri, uri);
    memcpy(cache[index].content, content, len);
    cache[index].valid = 1;
    cache[index].len = len;
    cache_update_LRU(index);
}

void cache_update_LRU(int index)
{
    /* 将 index 的 LRUtag 置为 0，并将其余块的 LRUtag 加一 */
    for (int i = 0; i < MAX_CACHE_BLK; ++i)
    {
        if (index == i)
        {
            cache[index].LRUtag = 0;
            writer_unlock(index);
            continue;
        }
        writer_lock(i);
        cache[i].LRUtag++;
        writer_unlock(i);
    }
}
void cache_debug()
{
    dbg_printf("CACHE DEBUG!\n");
    for (int i = 0; i < MAX_CACHE_BLK; ++i)
    {
        reader_lock(i);
        dbg_printf("uri:%s ;index:%d ;valid:%d ;LRUtag:%d ;len:%d ;\n",
                   cache[i].uri, i, cache[i].valid, cache[i].LRUtag, cache[i].len);
        reader_unlock(i);
    }
}

void reader_lock(int i)
{
    P(&rwlock[i].base_lock);
    rwlock[i].readcnt++;
    if (rwlock[i].readcnt == 1)
        P(&rwlock[i].write_lock);
    V(&rwlock[i].base_lock);
}
void reader_unlock(int i)
{
    P(&rwlock[i].base_lock);
    rwlock[i].readcnt--;
    if (rwlock[i].readcnt == 0)
        V(&rwlock[i].write_lock);
    V(&rwlock[i].base_lock);
}
void writer_lock(int i)
{
    P(&rwlock[i].write_lock);
}
void writer_unlock(int i)
{
    V(&rwlock[i].write_lock);
}