/*寿晨宸 2100012945*/

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <limits.h>
#include "cachelab.h"
#define HIT 0
#define MISS 1
#define EVICTION 2
#define ERROR -1
void mallocCache(void);              // 为模拟的cache动态分配空间，并初始化缓存的内容
void freeCache(void);                // 释放分配给cache的空间
void printHelpInfo(int);             // 打印help和error的信息
void cacheUpdate(long, int, char *); // 更新缓存状态
void timeUpdate(void);               // 更新时间戳
void loadTrace(void);                // 读入trace文件，并做相关的操作
// h,v分别标识help/verbose mode，s为索引位的位数，S为组数，E为行数，b为块偏移的大小
int h = 0, v = 0, s = 0, S = 0, E = 0, b = 0;
char t[1000];
FILE *fp = NULL;
// 对缓存进行的操作，总共有3种可能
char actionS[3][20] = {" hit", " miss", " eviction"};
// 计数器
int hitCnt = 0, missCnt = 0, evictionCnt = 0;
// 模拟的缓存行
typedef struct
{
    long valid;  // 有效位
    long tag;    // 标记位
    long LRUtag; // 时间戳，用于标识最后一次访问时间
    // 对较大的数据，需要long!!!!!!!!
} cacheLine, *cacheSet, **cache;
cache myCache = NULL; // 程序中模拟的缓存
void mallocCache()    // 为模拟的cache动态分配空间，并初始化缓存的内容
{
    myCache = (cache)malloc(sizeof(cacheSet) * S); // 分配S组的空间
    for (int i = 0; i < S; ++i)
    {
        myCache[i] = (cacheSet)malloc(sizeof(cacheLine) * E);
        for (int j = 0; j < E; ++j)
        {
            myCache[i][j].LRUtag = -1;
            myCache[i][j].tag = -1;
            myCache[i][j].valid = 0;
        }
    }
    return;
}
void freeCache() // 释放分配给cache的空间
{
    for (int i = 0; i < S; ++i)
    {
        free(myCache[i]);
        // myCache[i]=NULL;
    }
    free(myCache);
    // myCache=NULL;
    return;
}
void printHelpInfo(int error)
// 当输入的参数为-h或输入的数据不符合要求时，调用此函数，打印help的信息
{
    if (error == ERROR)
        printf("./csim-ref: Missing required command line argument\n");

    printf(
        "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
        "Options:\n"
        "  -h         Print this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n"
        "\n"
        "Examples:\n"
        "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
        "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
    return;
}
void cacheUpdate(long add, int size, char *act) // 更新缓存状态
{
    // printf("%x %d %s\n",add,size,act);
    long sNum = (add >> b) & (~((~0U) << s)); // 组索引
    long tag = add >> (b + s);                // 标识位

    for (int j = 0; j < E; ++j) // 检索，若标记位匹配，则命中
    {
        if (myCache[sNum][j].tag == tag)
        {
            myCache[sNum][j].LRUtag = -1;
            act = strcat(act, actionS[HIT]);
            hitCnt++;
            return;
        }
    }
    // 若未命中，查看有无空行
    for (int j = 0; j < E; ++j)
    {
        if (myCache[sNum][j].valid == 0)
        {
            myCache[sNum][j].LRUtag = -1;
            myCache[sNum][j].tag = tag;
            myCache[sNum][j].valid = 1;
            act = strcat(act, actionS[MISS]);
            missCnt++;
            // 若有空行，则直接填入
            return;
        }
    }
    // 若无空行，就要驱逐
    missCnt++;
    strcat(act, actionS[MISS]);
    evictionCnt++;
    strcat(act, actionS[EVICTION]);
    long maxLRUTag = LONG_MIN, maxLRUIndex = -1; // LRU策略应该选中的行的信息
    for (int j = 0; j < E; ++j)
    {
        if (myCache[sNum][j].LRUtag >= maxLRUTag)
        {
            maxLRUIndex = j;
            maxLRUTag = myCache[sNum][j].LRUtag;
        }
    }
    myCache[sNum][maxLRUIndex].LRUtag = -1;
    myCache[sNum][maxLRUIndex].tag = tag;

    return;
}
void timeUpdate() // 更新时间戳
{
    for (int i = 0; i < S; ++i)
        for (int j = 0; j < E; ++j)
        {
            if (myCache[i][j].valid == 1)
                myCache[i][j].LRUtag++;
        }
    return;
}
void loadTrace() // 读入trace文件，并做相关的操作
{
    // trace文件的每一行格式为 [operation][address][size]
    // 其中，operation有I，M，L，S
    // I可忽略，M表示modify(load+store)，L表示load，S表示store
    fp = fopen(t, "r");
    char operation;
    long address;
    int size;
    char act[40];
    memset(act, 0, sizeof(act));
    // fscanf,成功返回读入的参数的个数，失败返回EOF(-1)
    while (fscanf(fp, " %c %lx,%d\n", &operation, &address, &size) > 0)
    {
        memset(act, 0, sizeof(act));
        // fprintf(stdout, "%c %xu,%d\n", operation, address, size);
        if (operation == 'I')
            continue;
        if (operation == 'M')
            cacheUpdate(address, size, act); // 如果是M操作，需要更新两次
        cacheUpdate(address, size, act);
        timeUpdate();
        if (v)
            fprintf(stdout, "%c %lx,%d%s\n", operation, address, size, act);
    }
    fclose(fp);
    printSummary(hitCnt, missCnt, evictionCnt);
    return;
}
int main(int argc, char *argv[])
{
    int option; // 记录getopt的返回值
    while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        // 读取输入命令的各参数，并判断各参数是否合规，若不合规，
        // 则输出错误提示和helpinfo，并直接终止程序
        switch (option)
        {
        case 'h':
            h = 1;
            printHelpInfo(0);
            return 0;
        case 'v':
            v = 1;
            break;
        case 's':
        {
            s = atoi(optarg);
            if (s <= 0)
            {
                printHelpInfo(ERROR);
                return 0;
            }
            S = 1 << s; // 与此同时，计算组数S=2^s
            break;
        }
        case 'E':
        {
            E = atoi(optarg);
            if (E <= 0)
            {
                printHelpInfo(ERROR);
                return 0;
            }
            break;
        }
        case 'b':
        {
            b = atoi(optarg);
            if (b <= 0)
            {
                printHelpInfo(ERROR);
                return 0;
            }
            break;
        }
        case 't':
        {
            strcpy(t, optarg);
            fp = fopen(t, "r");
            if (fp == NULL)
            {
                printHelpInfo(ERROR);
                return 0;
            }
            break;
        }
        default:
            printHelpInfo(ERROR);
            return 0;
        }
    }
    // printf("%d %d %d %d %d %s", h, v, s, E, b, t);
    mallocCache();
    loadTrace();
    freeCache();
    return 0;
}