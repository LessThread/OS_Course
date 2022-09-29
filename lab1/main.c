#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

// Simplifed xv6 shell.

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
    int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
    int type;              // ' '执行命令
    char* argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
    int type;          // < or > 重定向命令
    struct cmd* cmd;   // the command to be run (e.g., an execcmd)
    char* file;        // the input/output file
    int flags;         // flags for open() indicating read or write
    int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
    int type;          // | 管道命令
    struct cmd* left;  // left side of pipe
    struct cmd* right; // right side of pipe
};

int fork1(void);  // Fork but exits on failure.
struct cmd* parsecmd(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd* cmd)//cmd 的核心驱动函数,根据不同的符号进行解析命令
{
    //printf("*we called runcmd*");
    int p[2], r;
    struct execcmd* ecmd;
    struct pipecmd* pcmd;
    struct redircmd* rcmd;

    if (cmd == 0)
        _exit(0);

    switch (cmd->type) {
    default:
        fprintf(stderr, "unknown runcmd\n");
        _exit(-1);

    case ' ':
        ecmd = (struct execcmd*)cmd;
        if (ecmd->argv[0] == 0)
            _exit(0);
        fprintf(stderr, "[@]:exec case\n");
        // Your code here ...
        break;

    case '>':
    case '<':
        rcmd = (struct redircmd*)cmd;
        fprintf(stderr, "redir not implemented\n");
        // Your code here ...
        runcmd(rcmd->cmd);
        break;

    case '|':
        pcmd = (struct pipecmd*)cmd;
        fprintf(stderr, "pipe not implemented\n");
        // Your code here ...
        break;
    }
    _exit(0);
}



int
getcmd(char* buf, int nbuf)
{
    //如果是标准输入环境则显示
    //实现了路径追踪
    if (isatty(fileno(stdin))) 
    {
        char pPath[256] = { 0 };
        getcwd(pPath, 256);
        fprintf(stdout, "lyc@6.828_shell:");
        fprintf(stdout, pPath);
        fprintf(stdout, "$ ");
    }
    memset(buf, 0, nbuf);
    
    if (fgets(buf, nbuf, stdin) == 0)
    {
        return -1; // EOF

    }
    return 0;
}

int
main(void)
{
    static char buf[100]; //100字节的缓冲区
    int fd, r;

    // Read and run input commands.
    while (getcmd(buf, sizeof(buf)) >= 0) 
    {
        printf("getcmd : %s\n", buf);

        //cd命令处理
        if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') 
        {
            buf[strlen(buf) - 1] = 0;  // chop \n
            if (chdir(buf + 3) < 0)
                fprintf(stderr, "cannot cd %s\n", buf + 3);
            continue;
        }

        if (fork1() == 0)//fork()返回0则为子进程
        {
            runcmd(parsecmd(buf));//先解析再执行
        }
        wait(&r);
    }
    exit(0);
}

int
fork1(void)
{
    int pid;

    pid = fork();
    if (pid == -1)
        perror("fork");
    return pid;
}

struct cmd*
    execcmd(void)//初始化cmd实例，类型默认为‘ ’
{
    struct execcmd* cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = ' ';
    return (struct cmd*)cmd;
}

struct cmd*
    redircmd(struct cmd* subcmd, char* file, int type)
{
    struct redircmd* cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = type;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->flags = (type == '<') ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC;
    cmd->fd = (type == '<') ? 0 : 1;
    return (struct cmd*)cmd;
}

struct cmd*
    pipecmd(struct cmd* left, struct cmd* right)
{
    struct pipecmd* cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = '|';
    cmd->left = left;
    cmd->right = right;
    return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char** ps, char* es, char** q, char** eq)
{
    char* s;
    int ret;

    s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    if (q)
        *q = s;
    ret = *s;
    switch (*s) {
    case 0:
        break;
    case '|':
    case '<':
        s++;
        break;
    case '>':
        s++;
        break;
    default:
        ret = 'a';
        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if (eq)
        *eq = s;

    while (s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

int
peek(char** ps, char* es, char* toks)
{
    char* s;

    s = *ps;//s 指向ps字符串的首地址
    while (s < es && strchr(whitespace, *s))//搜索第一次出现空白符的位置
        s++;//char 指针加一
    *ps = s;//ps 为指向空字符的地址
    puts("\npeek here:");
    printf("*s: %s,strchr:%c & %s", *s, toks, *s);
    return *s && strchr(toks, *s);
}

struct cmd* parseline(char**, char*);
struct cmd* parsepipe(char**, char*);
struct cmd* parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char
* mkcopy(char* s, char* es)
{
    int n = es - s;
    char* c = malloc(n + 1);
    assert(c);
    strncpy(c, s, n);
    c[n] = 0;
    return c;
}

struct cmd*
    parsecmd(char* s)
{
    char* es;//es 是结束位
    struct cmd* cmd;

    es = s + strlen(s);//指针+地址 es->endof(S) es是尾指针

    printf("called parsecmd @s：%s\n", s);
    printf("called parsecmd: %p + %d = es:%p\n",s,strlen(s),es);

    cmd = parseline(&s, es);//调用解析行函数
    //&s 为字符串指针 ，es为结束地址（指针）

    peek(&s, es, "");
    if (s != es) {
        fprintf(stderr, "leftovers: %s\n", s);
        exit(-1);
    }


    return cmd;
}

struct cmd*
    parseline(char** ps, char* es)
{
    puts("called parseline\n");
    struct cmd* cmd;
    cmd = parsepipe(ps, es);//调用解析管道符
    return cmd;
}

struct cmd*
    parsepipe(char** ps, char* es)
{
    struct cmd* cmd;

    cmd = parseexec(ps, es);
    if (peek(ps, es, "|")) {
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd*
    parseredirs(struct cmd* cmd, char** ps, char* es)
{
    puts("parseredirs called\n");
    printf("%s:",ps);
    int tok;
    char* q, * eq;

    while (peek(ps, es, "<>")) {
        tok = gettoken(ps, es, 0, 0);
        if (gettoken(ps, es, &q, &eq) != 'a') {
            fprintf(stderr, "missing file for redirection\n");
            exit(-1);
        }
        switch (tok) {
        case '<':
            cmd = redircmd(cmd, mkcopy(q, eq), '<');
            break;
        case '>':
            cmd = redircmd(cmd, mkcopy(q, eq), '>');
            break;
        }
    }
    return cmd;
}

struct cmd*
    parseexec(char** ps, char* es)
{
    puts("parseexec called\n");
    char* q, * eq;
    int tok, argc;
    struct execcmd* cmd;
    struct cmd* ret;

    ret = execcmd();
    cmd = (struct execcmd*)ret;
    //初始化cmd命令变量

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while (!peek(ps, es, "|")) {
        if ((tok = gettoken(ps, es, &q, &eq)) == 0)
            break;
        if (tok != 'a') {
            fprintf(stderr, "syntax error\n");
            exit(-1);
        }
        cmd->argv[argc] = mkcopy(q, eq);
        argc++;
        if (argc >= MAXARGS) {
            fprintf(stderr, "too many args\n");
            exit(-1);
        }
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    return ret;
}
