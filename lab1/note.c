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
// Parser: lyc

#define MAXARGS 10

// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.

//规定命令类型
struct cmd {
  int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

//可执行命令，case为" "
struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // arguments to the command to be exec-ed 命令行参数
};

//重定向命令 case为<>
struct redircmd {
  int type;          // < or > 
  struct cmd *cmd;   // the command to be run (e.g., an execcmd) 被重定向的命令
  char *file;        // the input/output file 输入输出文件
  int flags;         // flags for open() indicating read or write 读写标记
  int fd;            // the file descriptor number to use for the file 
};

//管道命令（也是重定向了）case为|
struct pipecmd {
  int type;          // |
  struct cmd *left;  // left side of pipe 左管道命令
  struct cmd *right; // right side of pipe 右管道命令
};

int fork1(void);  // Fork but exits on failure. fork函数，失败时退出
struct cmd *parsecmd(char*);

// Execute cmd.  Never returns. 重复读取不会退出

//cmd 驱动函数，实现已解析命令的分类执行
void
runcmd(struct cmd *cmd)
{
  int p[2], r;
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    _exit(0);
  
  switch(cmd->type){
  default://异常处理
    fprintf(stderr, "unknown runcmd\n");
    _exit(-1);

  case ' '://（连续执行）可直接执行类命令
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      _exit(0);
    
    //fprintf(stderr, "exec not implemented\n");
    // Your code here ...
    break;

  case '>':
  case '<'://重定向类命令
    rcmd = (struct redircmd*)cmd;
    //fprintf(stderr, "redir not implemented\n");
    // Your code here ...
    runcmd(rcmd->cmd);
    break;

  case '|'://管道类命令
    pcmd = (struct pipecmd*)cmd;
    //fprintf(stderr, "pipe not implemented\n");
    // Your code here ...
    break;
  }    
  _exit(0);
}

int //实现了路径追踪(系统函数：getcwd)
getcmd(char *buf, int nbuf)
{
    /*
      if (isatty(fileno(stdin)))
      fprintf(stdout, "6.828$ ");
      memset(buf, 0, nbuf);
      if(fgets(buf, nbuf, stdin) == 0)
      return -1; // EOF
      return 0;
    */

    if (isatty(fileno(stdin))) //如果是标准输入环境则显示
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

int//主函数，实现创建子进程和循环读取
main(void)
{
  static char buf[100];
  int fd, r;

  // Read and run input commands.
  while(getcmd(buf, sizeof(buf)) >= 0){
    //实现cd命令，系统函数：chdir
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      // Clumsy but will have to do for now.
      // Chdir has no effect on the parent if run in the child.
      buf[strlen(buf)-1] = 0;  // chop \n
      if(chdir(buf+3) < 0)
        fprintf(stderr, "cannot cd %s\n", buf+3);
      continue;
    }
    if(fork1() == 0)
      runcmd(parsecmd(buf));//核心函数：runcmd（命令驱动函数）和parsecmd（命令语义解释器）
    wait(&r);
  }
  exit(0);
}

//fork函数的实现，作为系统函数的外壳函数，-1则抛出异常（perror是标准c库函数）
int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

struct cmd*//执行命令类别的构造函数
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*//执行重定向命令的构造函数
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC; //系统函数库：文件读写 fcntl.h
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}


struct cmd* //管道类命令的构造函数
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";//空白符号解析
char symbols[] = "<|>";//重定向符号解析

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
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
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
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
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}
