#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h> 
void build_daemon()
{
    // fork 出一个子进程。
    pid_t pid = fork();
    // 退出父进程。
    // 新进程就变成了孤儿进程，于是被init进程接收，这样新进程就和调用进程没有父子关系了。
    if (pid)
    {
        exit(0);
    }
    printf("%d\n",getpid());
    // 创建新会话。
    // 该子进程会成为新的会话和进程组的组长。
    // 同时，该新session不和任何tty关联。
    setsid();

    // 关掉从父进程继承的文件描述符。
    int max_fd = sysconf(_SC_OPEN_MAX);
    for (int i = 0; i < max_fd; ++i)
    {
        close(i);
    }

    // 重定向文件描述符 0, 1, 2 到 /dev/null(守护进程不再响应I/O)
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);

    // 设置文件创建权限掩码，不希望被父进程的掩码限制。
    umask(0);

    // 将当前工作目录设置为系统根目录。取消了对原工作目录的引用。使之不会影响磁盘的卸载。
    chdir("/");
}
int main()
{
    build_daemon();
    return 0;
}