# 第八章 ECF 讨论

老师、同学们好，今天由我来主持第八章 ECF 的讨论。有关于 ECF 的基本知识，之前的两位同学已经讲得相当完善了，我就不再赘述，今天我主要是补充一些 tricky 的细节和我找来的拓展内容。鉴于本人水平有限，大家且听且看，欢迎随时打断补充。

## 标准 I/O 的缓冲区与进程

我们先来看一个简短的例子。

```c
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
int main(void)
{   
   int i;
   for(i=0; i<2; i++){
      fork();
      printf("Hello!");
   }

   wait(NULL);
   wait(NULL);

   return 0;
}
```
猜猜看这个程序的输出是多少?

我们来捋一下这段代码的进程图。(不看 wait 的话)

```
                          --printf---
         --printf---fork-|
        |                 --printf---
---fork-|
        |                 --printf---   
         --printf---fork-|
                          --printf---
```

如果不考虑其他，只考虑这段代码的运行逻辑的话，应该输出6个 Hello!

但事实上，这段代码会输出8个 Hello! 这是怎么回事呢？

这其实与标准 I/O 自带的缓冲区有关。对于 c/c++ 中的 cin 和 getchar 等输入操作，我们从键盘上输入的字符先存到缓冲区里面，cin、getchar 等函数是从缓冲区里面读取输入；对于输出来说，程序将要输出的结果并不会直接输出到屏幕，而是先存放到输出缓存区，然后利用 cout、putchar 等函数将缓冲区中的内容输出到屏幕上。

因此，标准 I/O 并非直接进行输入输出。只有当数据被刷出缓冲区时，才会进行真正的输入输出操作。

通常来讲，标准 I/O 对应终端设备（如屏幕）时，都是行缓冲的。只有在写了一行之后才进行实际 I/O 操作。
```c
#include <stdio.h>
int main(int argc, char *argv[])
{
	printf("Hello");
	while(1);
	return 0;
}
```
比如上面这个程序，你永远不会看到它的输出，因为它的缓冲区永远不会被刷新。

那么，对于行缓冲的标准 I/O 而言，什么时候缓冲区会被刷新呢？

1. 遇到换行符 \n 或 EOF。以上文为例，只要改成 ```printf("Hello\n")``` ，就能产生输出。
2. 缓冲区填满。
   ```c
    #include <stdio.h>
    int main(int argc, char *argv[])
    {
        while(1)
	        printf("Hello");
	    while(1);
	    return 0;
    }
    ``` 
    缓冲区是有限的，只要缓冲区被填满，就会把多余的数据刷出缓冲区，形成输出。
3. 用 ```fflush``` 人为刷新缓冲区.
    ```c
    #include <stdio.h>
    int main(int argc, char *argv[])
    {
	    printf("Hello");
        fflush(stdout);
	    while(1);
	    return 0;
    }
    ```
4. 程序正常结束(return)。
    ```c
    #include <stdio.h>
    int main(int argc, char *argv[])
    {
	    printf("Hello");
	    return 0;
    }
    ```

现在，我们再回过头来观察最开始给出的例子，答案就很明显了。

父进程在创建子进程时，子进程会复制父进程上下文的一个副本，当然，也包括缓冲区！

而在给出的例子里，缓冲区只在程序结束时刷新。这就导致子进程会复制父进程未被清空的缓冲区。

```
                                     --buf:HelloHello---O
                 --buf:Hello---fork-|
                |                    --buf:HelloHello---O
buf:NULL---fork-|
                |                    --buf:HelloHello---O   
                 --buf:Hello---fork-|
                                     --buf:HelloHello---O
```

而多出的两个Hello，正是来自这里。 

这也是为什么在 shelllab 中每逢 printf 必有 fflush。

## fork 炸弹
在开始之前，我们先来看一条魔法一样的命令。
```bash
:(){ :|:& };:
```
拆开来看或许能更清楚些。
```bash
:()
{ 
    :|:& 
};
:
```
再不济，我们可以把它写成这个样子。
```bash
forkbomb()
{
    forkbomb | forkbomb&
};
forkbomb
```
也就是
```bash
forkbomb() { forkbomb | forkbomb& }; forkbomb
```
用 c 语言写出来的话，等效于下面这段程序，这就好懂多了。
```c
#include <unistd.h>
int main()
{
  while(1)
    fork();
  return 0;
}
```
很明显，上面这些程序的主要思路就是不断 fork 新的进程，不但在前台 fork 进程，也在后台 fork 进程。这种进程的递归式派生是指数式的，会以极快的速度占用进程表，同时消耗大量 CPU 资源和内存。当进程表饱和了之后，系统就无法运行新的程序了。就算有进程终止了，新 fork 的进程也会几乎立即填补空缺。同时，系统的响应时间也会大幅放缓，以至于无法响应任何命令。而这一切都会使系统几乎无法运作。

本人十分不幸的在 class machine 上尝试了一下 fork bomb ，后果大抵如下，请各位引以为戒。
 
当命令被键入之后，机器就开始变得卡顿。幸运的是，刚开始的时候，我还是能正常键入命令的。这大抵是我在实验之前设置了用户所能创建的进程数上限的缘故（ulimit -Hu 100)。但不幸的是，进程表很快就告罄了。现在我想输入命令，就要趁 fork bomb 不注意，重复数次见缝插针才能让 shell 搭理我一下。再过一段时间，屏幕的 I/O 就开始出问题，什么意思呢？众所周知，shell 解释器也是程序，而既然是程序，那大概是抢不过 fork bomb 的。从这里开始，我就只能盲打了——我只能看见命令的输出，但已经看不见命令本身了。到最后 shell 就彻底不理我了。
 
如何处理这个问题呢？
 
别尝试别的方法了，重启你的机子吧！

## nohup，会话，Linux守护进程和其他

在做 lab 的时候，我们会遇到一个奇怪的内置命令，叫做 nohup。这个命令有什么用？实现这个命令的基础是什么？以及有没有更好用的替代品？这是我们接下来要探讨的内容。

nohup，顾名思义，是 no hang up 的意思，即不要挂断。其使用格式为 ```nohup [COMMAND]```，作用是阻塞发向该进程的 SIGHUP 信号。在默认情况下，它会将 stdin 重定向到 /dev/null ，也就是说，该进程将不再接收标准输入。同时将 stdout 和 stderr 重定向到 nohup.out 或者用户通过参数指定的文件。

那么问题来了，什么时候，系统会向进程发送 SIGHUP 信号呢？

要回答这个问题，我们要先厘清一些概念。

会话(session)。会话是一组进程组(job)的集合。我们常见的 Linux session 一般是指 shell session。当我们打开一个新的终端时，总会创建一个新的 shell session。一般情况下，若 session 与 tty 关联，那么它们是一一对应的。

通俗点讲，通常一个终端窗口对应一个会话。

会话是由会话中的第一个进程创建的，一般情况下是打开终端时创建的 shell 进程。该进程也叫 session 的领头进程。Session 中领头进程的 PID 也就是 session 的 SID。

如我们知道的那样，session 中仅有一个 job 在前台运行，剩余的 job 都作为后台进程。

session 的组织结构大抵如下图所示。

```
         -- job1--leader process(shell)
        |
session- -- foreground job -- foreground processes
        |
        |                      --job1--processes
        |                     |
         -- background jobs -- --job2--processes
                              |
                               --....
```
当 session 中领头进程退出（shell 中输入 exit；kill），或者 session 中所有进程都结束（网络断开/断点/强行关闭终端）时，session 就消亡了。

而当 session 消亡时，就会向 session 中的所有进程发送 SIGHUP 信号。系统对SIGHUP信号的默认处理是终止收到该信号的进程。

现在，我们就知道什么时候，系统会向进程发送 SIGHUP 信号；我们也大致能理解 nohup 命令究竟有什么作用了：无论你因为什么原因手误关闭了终端，或者因为网络卡顿不得不断开与服务器的连接，你都可以用 nohup 指令，使原本因为会话终止而被迫终止的进程（譬如 apt install）继续运行下去。

而 lab 中我们手动实现的 nohup，其作用就是使在 tsh 中启动的进程能够在退出 tsh 之后继续运行下去。

可以看出，nohup 的功能还是很强大的。但是，它也有自己的局限性，譬如，不管怎么说，用 nohup 执行的进程与 session 还是有千丝万缕的关系。那我们能不能实现一个完全脱离 session 而存在的进程呢？

这就是 Linux 守护进程，即 deamon process 的由来。它的 PPID 为 1，始终在后台运行，且不受任何终端控制。我们在使用 ps 命令的时候经常会看到一些 tty 显示为 ??? 的奇怪进程，这些进程通常就是守护进程，用于保护我们的程序/服务的正常运行。

那要如何创建一个守护进程呢？大概步骤参见下面这段代码。
```c
void daemon() {
    // fork 出一个子进程。
    pid_t pid = fork();
    
    // 退出父进程。
    //新进程就变成了孤儿进程，于是被init进程接收，这样新进程就和调用进程没有父子关系了。
    if(pid) {
        exit(0);
    }

    // 创建新会话。
    // 该子进程会成为新的会话和进程组的组长。
    // 同时，该新session不和任何tty关联。
    setsid();

    
    // 关掉从父进程继承的文件描述符。
    int max_fd = sysconf(_SC_OPEN_MAX);
    for(int i = 0; i < max_fd; ++i) {
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
```

通过上面的这些知识，我们就能做到使进程不受会话终止的干扰，使远程服务器一直平稳运行。但一直用手搓进程和会话，还是显得有些麻烦了，这时候，终端复用器就成了比较方便的选择。

它们可以在当前 session 里面，新建另一个 session。这样的话，当前 session 一旦结束，不影响其他 session。而且，以后重新登录，还可以再连上早先新建的 session。

比较常用的终端复用器有 tmux 和 GNU screen，通过它们封装好的快捷键，用户可以便捷地操作会话。有兴趣的同学可以了解一下。

今天的讨论到此为止，欢迎各位指正，谢谢大家！
