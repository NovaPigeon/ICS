/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 * 寿晨宸 2100012945
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Parsing states */
#define ST_NORMAL 0x0  /* next token is an argument */
#define ST_INFILE 0x1  /* next token is the input file */
#define ST_OUTFILE 0x2 /* next token is the output file */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t job_list[MAXJOBS]; /* The job list */

struct cmdline_tokens
{
    int argc;            /* Number of arguments */
    char *argv[MAXARGS]; /* The arguments list */
    char *infile;        /* The input file */
    char *outfile;       /* The output file */
    enum builtins_t
    { /* Indicates if argv[0] is a builtin command */
      BUILTIN_NONE,
      BUILTIN_QUIT,
      BUILTIN_JOBS,
      BUILTIN_BG,
      BUILTIN_FG,
      BUILTIN_KILL,
      BUILTIN_NOHUP
    } builtins;
};

/* End global variables */

/* Function prototypes */
void eval(char *cmdline);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, struct cmdline_tokens *tok);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *job_list);
int maxjid(struct job_t *job_list);
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *job_list, pid_t pid);
pid_t fgpid(struct job_t *job_list);
struct job_t *getjobpid(struct job_t *job_list, pid_t pid);
struct job_t *getjobjid(struct job_t *job_list, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *job_list, int output_fd);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
ssize_t sio_put(const char *fmt, ...);
void sio_error(char s[]);

typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE]; /* cmdline for fgets */
    int emit_prompt = 1;   /* emit prompt (default), prompt="tsh>" */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */
    Signal(SIGTTIN, SIG_IGN);
    Signal(SIGTTOU, SIG_IGN);

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(job_list);

    /* Execute the shell's read/eval loop */
    while (1)
    {

        if (emit_prompt)
        {
            printf("%s", prompt);
            /* 清空stdout的缓冲 */
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        {
            /* End of file (ctrl-d) */
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            exit(0);
        }

        /* Remove the trailing newline */
        cmdline[strlen(cmdline) - 1] = '\0';

        /* Evaluate the command line */
        eval(cmdline);

        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline)
{
    int bg;            /* should the job run in bg or fg? */
    int fd_in, fd_out; /* I/O 文件 */
    int buildin_type;  /* 命令的类型 */
    struct cmdline_tokens tok;
    sigset_t mask_all, mask_hup, mask_three, prev; /* block 的掩码 */
    pid_t pid;                                     /* 用于记录pid */

    /*  设置阻塞的掩码 */
    sigfillset(&mask_all);
    sigemptyset(&mask_three);
    sigaddset(&mask_three, SIGINT);
    sigaddset(&mask_three, SIGTSTP);
    sigaddset(&mask_three, SIGCHLD);
    sigemptyset(&mask_hup);
    sigaddset(&mask_hup, SIGHUP);

    /* Parse command line */
    bg = parseline(cmdline, &tok);
    buildin_type = tok.builtins;

    if (bg == -1) /* parsing error */
        return;
    if (tok.argv[0] == NULL) /* ignore empty lines */
        return;

    /* I/O 重定向 */
    if (tok.infile != NULL)
    {
        fd_in = open(tok.infile, O_RDONLY, 0);
        if (fd_in < 0)
        {
            sio_puts("Error: ");
            sio_puts(tok.infile);
            sio_puts(" No such file or directory\n");
            return;
        }
    }
    else
        fd_in = STDIN_FILENO;

    if (tok.outfile != NULL)
    {
        fd_out = open(tok.outfile, O_WRONLY | O_CREAT | O_TRUNC, 0);
        // 实际操作中，若文件不存在，shell会新建该文件
    }
    else
        fd_out = STDOUT_FILENO;

    if (buildin_type == BUILTIN_NOHUP || buildin_type == BUILTIN_NONE)
    {
        /* 这两个操作都需要用到非内置的命令行参数 */
        sigprocmask(SIG_BLOCK, &mask_three, &prev);
        int is_nohup = 0; /* 标记命令的类型是否是 nohup */
        if ((pid = fork()) == 0)
        {
            setpgid(0, 0);                         // 将当前子进程加入以其pid为id的新的进程组中，避免tsh被SIGTSTP之类的信号影响。
            sigprocmask(SIG_SETMASK, &prev, NULL); // 子进程解锁阻塞的信号
            if (buildin_type == BUILTIN_NOHUP)
            {
                sigprocmask(SIG_SETMASK, &mask_hup, &prev);
                is_nohup = 1;
            }
            // 在子进程中重定向，就不必考虑重定向对其他进程的干扰
            if (tok.infile != NULL)
                dup2(fd_in, STDIN_FILENO);
            if (tok.outfile != NULL)
                dup2(fd_out, STDOUT_FILENO);
            if (execve(tok.argv[is_nohup], tok.argv + is_nohup, environ) < 0)
            {
                sio_puts(tok.argv[0]);
                sio_puts(": Command not found.\n");
                exit(0);
            }
        }
        sigprocmask(SIG_SETMASK, &mask_all, NULL); // 阻塞所有信号，防止addjob被干扰
        if (!bg)
        {
            /* 如果在前台运行 */
            addjob(job_list, pid, FG, cmdline);
            while (pid == fgpid(job_list))
                sigsuspend(&prev);
        }
        else
        {
            /* 如果在后台运行 */
            addjob(job_list, pid, BG, cmdline);
            sio_put("[%d] (%d) ", pid2jid(pid), pid);
            sio_puts(cmdline);
            sio_puts("\n");
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    else if (buildin_type == BUILTIN_BG || buildin_type == BUILTIN_FG)
    {
        /* 前后台切换命令是相似的，可以同时处理 */
        /*
         *   The bg/fg job command restarts job by sending it a SIGCONT signal, and then runs it in the
         *   background/foreground. The job argument can be either a PID or a JID.
         */
        if (tok.argc != 2 || tok.argv[1] == NULL)
        {
            sio_puts(tok.argv[0]);
            sio_puts(": argument must be a PID or %jobid\n");
            return;
        }
        struct job_t *job = NULL;   /* 标识id指向的任务 */
        char *optstr = tok.argv[1]; /* %JID 或 PID */
        if (optstr[0] == '%')
        {
            /* %job */
            // JID 不为0，当输入的字符串不为纯数字时，atoi返回0，两者不会冲突，可用atoi判断输入的JID是否合法
            int jid = atoi(optstr + 1);
            if (jid == 0)
            {
                sio_puts(tok.argv[0]);
                sio_puts(" argument must be a PID or %jobid\n");
                return;
            }
            job = getjobjid(job_list, jid);
            if (job == NULL)
            {
                // job不存在
                sio_put("%%%d: No such job\n", jid);
                return;
            }
            /*
            sio_put("[%d] (%d) ", job->jid, job->pid);
            sio_puts(job->cmdline);
            sio_puts("\n");
            */
        }
        else
        {
            /* PID */
            // PID 不为0，当输入的字符串不为纯数字时，atoi返回0，两者不会冲突，可用atoi判断输入的PID是否合法
            int pid = atoi(optstr);
            if (pid == 0)
            {
                sio_puts(tok.argv[0]);
                sio_puts(" argument must be a PID or %jobid\n");
                return;
            }
            job = getjobpid(job_list, pid);
            if (job == NULL)
            {
                // 进程不存在
                sio_put("(%d): No such process\n", pid);
                return;
            }
        }
        int pid = job->pid;
        sigprocmask(SIG_BLOCK, &mask_all, &prev); // 阻塞所有信号
        if (buildin_type == BUILTIN_BG)
        {
            if (job->state == ST)
            {
                job->state = BG;
                kill(-(job->pid), SIGCONT);
                sio_put("[%d] (%d) ", job->jid, job->pid);
                sio_puts(job->cmdline);
                sio_puts("\n");
            }
        }
        if (buildin_type == BUILTIN_FG)
        {
            if (job->state == ST)
            {
                job->state = FG;
                kill(-job->pid, SIGCONT);
                while (pid == fgpid(job_list))
                    sigsuspend(&prev);
            }
            else if (job->state == BG)
            {
                job->state = FG;
                while (job->pid == fgpid(job_list))
                    sigsuspend(&prev);
            }
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }
    else if (buildin_type == BUILTIN_KILL)
    {
        /*
         *   The kill job command kills a job in the job list, or a process group by sending each relevant
         *   process a SIGTERM signal. The job argument can be either a PID or a JID. Note that if you
         *   get a negative job argument, such as kill %-1 or kill -15213, your shell should kill the
         *   process group of the job with a JID of %-JID, or of the job with a PID of -PID. If the process
         *   group does not exist, your shell should print ”%JID: No such process group” or ”(PID): No such
         *   process group”, where JID and PID should be replaced by the command line argument. On the
         *   other hand, if the job argument is positive, your shell should kill the corresponding job. If the
         *   job does not exist, your shell should print ”%JID: No such job” or ”(PID): No such process”.
         *   Play with the reference shell to check the details and gain intuition.
         */
        struct job_t *job = NULL;
        char *optstr = tok.argv[1];
        if (optstr == NULL)
        {
            sio_puts("kill command requires PID or %jobid argument\n");
            return;
        }
        if (optstr[0] == '%')
        {
            /* %JID */
            int jid = atoi(optstr + 1);
            if (jid == 0)
            {
                sio_puts("kill: argument must be a PID or %jobid\n");
                return;
            }
            job = getjobjid(job_list, abs(jid));
            if (job == NULL)
            {
                if (jid >= 0)
                    sio_put("%%%d: No such job\n", jid);
                else
                    sio_put("%%%d: No such process group\n", jid);
                return;
            }
            if (jid >= 0)
                kill(job->pid, SIGTERM);
            else
                kill(-job->pid, SIGTERM);
        }
        else
        {
            int pid = atoi(optstr);
            if (pid == 0)
            {
                sio_puts("kill: argument must be a PID or %jobid\n");
                return;
            }
            job = getjobpid(job_list, abs(pid));
            if (job == NULL)
            {
                if (pid >= 0)
                    sio_put("(%d): No such process\n", pid);
                else
                    sio_put("(%d): No such process group\n", pid);
                return;
            }
            kill(job->pid, SIGTERM);
        }
    }
    else if (buildin_type == BUILTIN_JOBS)
    {
        listjobs(job_list, fd_out);
    }
    else if (buildin_type == BUILTIN_QUIT)
    {
        if (tok.infile)
            close(fd_in);
        if (tok.outfile)
            close(fd_out);
        exit(0);
    }
    /* 关闭打开的文件，因为只在子进程中重定向，所以不必还原 */
    if (tok.infile)
        close(fd_in);
    if (tok.outfile)
        close(fd_out);
    return;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Parameters:
 *   cmdline:  The command line, in the form:
 *
 *                command [arguments...] [< infile] [> oufile] [&]
 *
 *   tok:      Pointer to a cmdline_tokens structure. The elements of this
 *             structure will be populated with the parsed tokens. Characters
 *             enclosed in single or double quotes are treated as a single
 *             argument.
 * Returns:
 *   1:        if the user has requested a BG job
 *   0:        if the user has requested a FG job
 *  -1:        if cmdline is incorrectly formatted
 *
 * Note:       The string elements of tok (e.g., argv[], infile, outfile)
 *             are statically allocated inside parseline() and will be
 *             overwritten the next time this function is invoked.
 */
int parseline(const char *cmdline, struct cmdline_tokens *tok)
{

    static char array[MAXLINE];        /* holds local copy of command line */
    const char delims[10] = " \t\r\n"; /* argument delimiters (white-space) */
    char *buf = array;                 /* ptr that traverses command line */
    char *next;                        /* ptr to the end of the current arg */
    char *endbuf;                      /* ptr to end of cmdline string */
    int is_bg;                         /* background job? */

    int parsing_state; /* indicates if the next token is the
                          input or output file */

    if (cmdline == NULL)
    {
        (void)fprintf(stderr, "Error: command line is NULL\n");
        return -1;
    }

    (void)strncpy(buf, cmdline, MAXLINE);
    endbuf = buf + strlen(buf);

    tok->infile = NULL;
    tok->outfile = NULL;

    /* Build the argv list */
    parsing_state = ST_NORMAL;
    tok->argc = 0;

    while (buf < endbuf)
    {
        /* Skip the white-spaces */
        buf += strspn(buf, delims);
        if (buf >= endbuf)
            break;

        /* Check for I/O redirection specifiers */
        if (*buf == '<')
        {
            if (tok->infile)
            {
                (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
                return -1;
            }
            parsing_state |= ST_INFILE;
            buf++;
            continue;
        }
        if (*buf == '>')
        {
            if (tok->outfile)
            {
                (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
                return -1;
            }
            parsing_state |= ST_OUTFILE;
            buf++;
            continue;
        }

        if (*buf == '\'' || *buf == '\"')
        {
            /* Detect quoted tokens */
            buf++;
            next = strchr(buf, *(buf - 1));
        }
        else
        {
            /* Find next delimiter */
            next = buf + strcspn(buf, delims);
        }

        if (next == NULL)
        {
            /* Returned by strchr(); this means that the closing
               quote was not found. */
            (void)fprintf(stderr, "Error: unmatched %c.\n", *(buf - 1));
            return -1;
        }

        /* Terminate the token */
        *next = '\0';

        /* Record the token as either the next argument or the i/o file */
        switch (parsing_state)
        {
        case ST_NORMAL:
            tok->argv[tok->argc++] = buf;
            break;
        case ST_INFILE:
            tok->infile = buf;
            break;
        case ST_OUTFILE:
            tok->outfile = buf;
            break;
        default:
            (void)fprintf(stderr, "Error: Ambiguous I/O redirection\n");
            return -1;
        }
        parsing_state = ST_NORMAL;

        /* Check if argv is full */
        if (tok->argc >= MAXARGS - 1)
            break;

        buf = next + 1;
    }

    if (parsing_state != ST_NORMAL)
    {
        (void)fprintf(stderr,
                      "Error: must provide file name for redirection\n");
        return -1;
    }

    /* The argument list must end with a NULL pointer */
    tok->argv[tok->argc] = NULL;

    if (tok->argc == 0) /* ignore blank line */
        return 1;

    if (!strcmp(tok->argv[0], "quit"))
    { /* quit command */
        tok->builtins = BUILTIN_QUIT;
    }
    else if (!strcmp(tok->argv[0], "jobs"))
    { /* jobs command */
        tok->builtins = BUILTIN_JOBS;
    }
    else if (!strcmp(tok->argv[0], "bg"))
    { /* bg command */
        tok->builtins = BUILTIN_BG;
    }
    else if (!strcmp(tok->argv[0], "fg"))
    { /* fg command */
        tok->builtins = BUILTIN_FG;
    }
    else if (!strcmp(tok->argv[0], "kill"))
    { /* kill command */
        tok->builtins = BUILTIN_KILL;
    }
    else if (!strcmp(tok->argv[0], "nohup"))
    { /* kill command */
        tok->builtins = BUILTIN_NOHUP;
    }
    else
    {
        tok->builtins = BUILTIN_NONE;
    }

    /* Should the job run in the background? */
    if ((is_bg = (*tok->argv[tok->argc - 1] == '&')) != 0)
        tok->argv[--tok->argc] = NULL;

    return is_bg;
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP, SIGTSTP, SIGTTIN or SIGTTOU signal. The
 *     handler reaps all available zombie children, but doesn't wait
 *     for any other currently running children to terminate.
 */
void sigchld_handler(int sig)
{
    /* 处理SIGCHID */
    int old_errno = errno;   /* 保存errno状态 */
    pid_t pid;               /* PID */
    int stat;                /* 进程的退出状态 */
    sigset_t mask_all, prev; /* block */
    sigfillset(&mask_all);
    while ((pid = waitpid(-1, &stat, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
    {
        sigprocmask(SIG_BLOCK, &mask_all, &prev);
        if (WIFSIGNALED(stat))
        {
            // 因为未被捕获的信号终止
            int sig = WTERMSIG(stat); // 获取该信号
            sio_put("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, sig);
            deletejob(job_list, pid);
        }
        else if (WIFSTOPPED(stat))
        {
            // 引起返回的子进程是停止的
            int sig = WSTOPSIG(stat);
            struct job_t *job = getjobpid(job_list, pid);
            job->state = ST;
            sio_put("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, sig);
        }
        else if(WIFEXITED(stat))
            deletejob(job_list, pid);
        else
        {   
            // 处理 runtrace 中的 SIGNAL 
            kill(pid,SIGCONT);
            struct job_t *job = getjobpid(job_list, pid);
            job->state=BG;
        }
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }

    errno = old_errno;
    return;
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
    int old_errno = errno;   /* 保存errno状态 */
    pid_t pid;               /* PID */
    sigset_t mask_all, prev; /* block */
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev);
    pid = fgpid(job_list);
    if (pid)
        kill(-pid, SIGINT);
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = old_errno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    int old_errno = errno;   /* 保存errno状态 */
    pid_t pid;               /* PID */
    sigset_t mask_all, prev; /* block */
    sigfillset(&mask_all);
    sigprocmask(SIG_BLOCK, &mask_all, &prev);
    pid = fgpid(job_list);
    if (pid)
        kill(-pid, SIGTSTP);
    sigprocmask(SIG_SETMASK, &prev, NULL);
    errno = old_errno;
    return;
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    sio_error("Terminating after receipt of SIGQUIT signal\n");
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *job_list)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&job_list[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *job_list)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid > max)
            max = job_list[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_list[i].pid == 0)
        {
            job_list[i].pid = pid;
            job_list[i].state = state;
            job_list[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(job_list[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n",
                       job_list[i].jid,
                       job_list[i].pid,
                       job_list[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *job_list, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (job_list[i].pid == pid)
        {
            clearjob(&job_list[i]);
            nextjid = maxjid(job_list) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *job_list)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].state == FG)
            return job_list[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t
    *
    getjobpid(struct job_t *job_list, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
            return &job_list[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *job_list, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid == jid)
            return &job_list[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
        {
            return job_list[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *job_list, int output_fd)
{
    int i;
    char buf[MAXLINE << 2];

    for (i = 0; i < MAXJOBS; i++)
    {
        memset(buf, '\0', MAXLINE);
        if (job_list[i].pid != 0)
        {
            sprintf(buf, "[%d] (%d) ", job_list[i].jid, job_list[i].pid);
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
            memset(buf, '\0', MAXLINE);
            switch (job_list[i].state)
            {
            case BG:
                sprintf(buf, "Running    ");
                break;
            case FG:
                sprintf(buf, "Foreground ");
                break;
            case ST:
                sprintf(buf, "Stopped    ");
                break;
            default:
                sprintf(buf, "listjobs: Internal error: job[%d].state=%d ",
                        i, job_list[i].state);
            }
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
            memset(buf, '\0', MAXLINE);
            sprintf(buf, "%s\n", job_list[i].cmdline);
            if (write(output_fd, buf, strlen(buf)) < 0)
            {
                fprintf(stderr, "Error writing to output file\n");
                exit(1);
            }
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/* Private sio_functions */
/* sio_reverse - Reverse a string (from K&R) */
static void sio_reverse(char s[])
{
    int c, i, j;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
    {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* sio_ltoa - Convert long to base b string (from K&R) */
static void sio_ltoa(long v, char s[], int b)
{
    int c, i = 0;

    do
    {
        s[i++] = ((c = (v % b)) < 10) ? c + '0' : c - 10 + 'a';
    } while ((v /= b) > 0);
    s[i] = '\0';
    sio_reverse(s);
}

/* sio_strlen - Return length of string (from K&R) */
static size_t sio_strlen(const char s[])
{
    int i = 0;

    while (s[i] != '\0')
        ++i;
    return i;
}

/* sio_copy - Copy len chars from fmt to s (by Ding Rui) */
void sio_copy(char *s, const char *fmt, size_t len)
{
    if (!len)
        return;

    for (size_t i = 0; i < len; i++)
        s[i] = fmt[i];
}

/* Public Sio functions */
ssize_t sio_puts(char s[]) /* Put string */
{
    return write(STDOUT_FILENO, s, sio_strlen(s));
}

ssize_t sio_putl(long v) /* Put long */
{
    char s[128];

    sio_ltoa(v, s, 10); /* Based on K&R itoa() */
    return sio_puts(s);
}

ssize_t sio_put(const char *fmt, ...) // Put to the console. only understands %d
{
    va_list ap;
    char str[MAXLINE]; // formatted string
    char arg[128];
    const char *mess = "sio_put: Line too long!\n";
    int i = 0, j = 0;
    int sp = 0;
    int v;

    if (fmt == 0)
        return -1;

    va_start(ap, fmt);
    while (fmt[j])
    {
        if (fmt[j] != '%')
        {
            j++;
            continue;
        }

        sio_copy(str + sp, fmt + i, j - i);
        sp += j - i;

        switch (fmt[j + 1])
        {
        case 0:
            va_end(ap);
            if (sp >= MAXLINE)
            {
                write(STDOUT_FILENO, mess, sio_strlen(mess));
                return -1;
            }

            str[sp] = 0;
            return write(STDOUT_FILENO, str, sp);

        case 'd':
            v = va_arg(ap, int);
            sio_ltoa(v, arg, 10);
            sio_copy(str + sp, arg, sio_strlen(arg));
            sp += sio_strlen(arg);
            i = j + 2;
            j = i;
            break;

        case '%':
            sio_copy(str + sp, "%", 1);
            sp += 1;
            i = j + 2;
            j = i;
            break;

        default:
            sio_copy(str + sp, fmt + j, 2);
            sp += 2;
            i = j + 2;
            j = i;
            break;
        }
    } // end while

    sio_copy(str + sp, fmt + i, j - i);
    sp += j - i;

    va_end(ap);
    if (sp >= MAXLINE)
    {
        write(STDOUT_FILENO, mess, sio_strlen(mess));
        return -1;
    }

    str[sp] = 0;
    return write(STDOUT_FILENO, str, sp);
}

void sio_error(char s[]) /* Put error message and exit */
{
    sio_puts(s);
    _exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t
    *
    Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}
