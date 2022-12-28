/* 寿晨宸 2100012945 */

#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

/* 总处理程序 */
void doit(int fd);
/* 解析 uri */
void parse_uri(char *uri, char *hostname, char *port, char *path);
/* 读取客户端发送的信息，处理后将其转发给服务器 */
void handle_client(rio_t *rp, int fd);
/* 读取服务器返回的内容，并将其转发给客户端 */
void handle_server(int serverfd, int clientfd,char* uri);
/* 线程处理程序 */
void *thread(void *vargp);

extern cache_t cache[MAX_CACHE_BLK];
extern rwlock_t rwlock[MAX_CACHE_BLK];

int main(int argc, char **argv)
{
    int listenfd;
    int* connfdp;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* 忽略信号 SIGPIPE */
    Signal(SIGPIPE,SIG_IGN);

    /* 检查输入是否合法 */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    /* 以端口 argv[1] 打开监听套接字 */
    listenfd = Open_listenfd(argv[1]);
    
    /* 初始化cache */
    cache_init();

    while (1)
    {
        clientlen = sizeof(clientaddr);
        /* connfdp 使用 malloc 以避免冲突 */
        connfdp=Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
                    port, MAXLINE, 0);
        dbg_printf("Accepted connection from (%s, %s)\n", hostname, port);
        Pthread_create(&tid,NULL,thread,connfdp);
    }

    return 0;
}
void doit(int connfd)
{
    int serverfd;
    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char requesthdrs[3*MAXLINE];
    char buf[MAXLINE];
    rio_t client_rio;
    
    /* 读取请求行 */
    Rio_readinitb(&client_rio, connfd);
    Rio_readlineb(&client_rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET"))
    {
        printf("The method is not implemented\n");
        return;
    }
    
    /* 如果cache命中了，则跳过与服务器的交互，直接将内容传回客户端 */
    int cache_index=cache_search_hit_blk(uri);
    if(cache_index!=-1)
    {
        reader_lock(cache_index);
        Rio_writen(connfd,cache[cache_index].content,cache[cache_index].len);
        reader_unlock(cache_index);
        cache_update_LRU(cache_index);
        return;
    }
    
    /* 解析uri */
    parse_uri(uri, hostname, port, path);
    dbg_printf("uri:%s\nhostname:%s\nport:%s\npath:%s\n",uri,hostname,port,path);
    
    /* 建立到服务器的连接 */
    serverfd = Open_clientfd(hostname, port);

    /* 向服务器转发请求行和请求头 */
    sprintf(requesthdrs, "GET %s HTTP/1.0\r\nHost: %s\r\n", path, hostname);
    Rio_writen(serverfd, requesthdrs, strlen(requesthdrs));
    handle_client(&client_rio, serverfd);
    /* 回送服务器返回的内容 */
    handle_server(serverfd, connfd,uri);
    
    Close(serverfd);

    return;
}
void parse_uri(char *uri, char *hostname, char *port, char *path)
{
    dbg_printf("parse uri:\n");
    char *host_startp, *host_endp;
    char *port_startp, *port_endp;
    char *path_startp, *path_endp;
    
    /* 解析 hostname */
    /* uri 中是否含 http://，若是，舍去之 */
    host_startp = strstr(uri, "//");
    if (host_startp == NULL)
        host_startp = uri;
    else
        host_startp += 2;
    host_endp = host_startp;
    while (1)
    {
        host_endp++;
        /* 寻找hostname的结束，若含 port，则以':'结束，否则，以'/'结束 */
        if (*host_endp == '/' || *host_endp == ':')
            break;
    }
    
    /* 取得hostname */
    strncpy(hostname, host_startp, host_endp - host_startp);
    dbg_printf("\thostname:%s\n",hostname);
    
    /* 解析 port */
    if (*host_endp == ':')
    {
        port_startp = host_endp + 1;
        port_endp = strstr(port_startp, "/");
        path_startp = port_endp + 1;
        strncpy(port, port_startp, port_endp - port_startp);
    }
    else
    {
        sprintf(port,"80");
        path_startp=host_endp+1;
    }
    
    /* 解析 path */
    dbg_printf("\tport:%s\n",port);
    path_endp = uri + strlen(uri) ;
    strncpy(path+1, path_startp, (int)(path_endp - path_startp) + 1);
    path[0]='/';//需要在path的开头添上'/'
    dbg_printf("\tpath:%s\n\n",path);
    return;
}

void handle_client(rio_t *rp, int fd)
{
    char buf[MAXLINE];
    /* 写标准的报头 */
    sprintf(buf, "%s", user_agent_hdr);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s", connection_hdr);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s", proxy_connection_hdr);
    Rio_writen(fd, buf, strlen(buf));
    /* 保留原有的报头（除了已经写了的） */
    while (1)
    {
        if (Rio_readlineb(rp, buf, MAXLINE)<=0)
            break;
        if (!strcmp(buf,"\r\n"))
            break;
        if (strstr(buf, "Host:") || strstr(buf, "User-Agent:") || strstr(buf, "Connection:") || strstr(buf, "Proxy-Connection:"))
            continue;
        Rio_writen(fd, buf, strlen(buf));
    }
    Rio_writen(fd, buf, strlen(buf));
    return;
}
void handle_server(int serverfd, int clientfd,char* uri)
{
    /* 读取服务器回送的数据，并将其写在客户端处  */
    char buf[MAXLINE];
    char content[MAX_OBJECT_SIZE];
    rio_t server_rio;
    size_t len;
    size_t size_sum=0;
    Rio_readinitb(&server_rio, serverfd);
    while (1)
    {
        len=Rio_readlineb(&server_rio,buf,MAXLINE);
        if(len==0)
            break;
        Rio_writen(clientfd, buf, len);
        size_sum+=len;
        if(size_sum<MAX_OBJECT_SIZE)
            memcpy(content+size_sum-len,buf,len);
    }
    cache_debug();
    if(size_sum<MAX_OBJECT_SIZE)
        cache_insert_blk(uri,content,size_sum);
    return;
}
void* thread(void* vargp)
{
    int connfd=*((int*)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}