#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define SOL
#define NBUCKET 5
#define NKEYS 100000

struct entry {
  int key;
  int value;
  struct entry *next;
};

struct entry *table[NBUCKET];//键值对表
int keys[NKEYS];//键的缓冲区
int nthread = 1;//默认线程数
volatile int done;//join判值
pthread_mutex_t lock[NBUCKET];//互斥锁组：给哈希表的每个表头上锁

double
now()//计时函数，实现不重要
{
 struct timeval tv;
 gettimeofday(&tv, 0);
 return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static void
print(void)
{
  int i;
  struct entry *e;
  for (i = 0; i < NBUCKET; i++) {
    printf("%d: ", i);
    for (e = table[i]; e != 0; e = e->next) {
      printf("%d ", e->key);
    }
    printf("\n");
  }
}

static void 
insert(int key, int value, struct entry **p, struct entry *n)
{
  struct entry *e = malloc(sizeof(struct entry));//实例化结构体
  e->key = key;
  e->value = value;
  e->next = n;
  *p = e;//将实例的指针挂载在表上
}

static 
void put(int key, int value)
{
  int i = key % NBUCKET;//值取5模（哈希表操作）
  pthread_mutex_lock(&lock[i]);
  insert(key, value, &table[i], table[i]);//在表中插入键值对
  pthread_mutex_unlock(&lock[i]);
}

static struct entry*
get(int key)
{
  struct entry *e = 0;
  //若
  for (e = table[key % NBUCKET]; e != 0; e = e->next) {
    if (e->key == key) break;
  }
  return e;
}

static void *
thread(void *xa)
{
  long n = (long) xa;//xa 实质上是句柄
  int i;//迭代器
  int b = NKEYS/nthread;//单线程操作量
  int k = 0;//miss计数
  double t1, t0;

  //  printf("b = %d\n", b);
  t0 = now();
  for (i = 0; i < b; i++) {
    // printf("%d: put %d\n", n, b*n+i);
    //读key池应该是不会发生资源抢占的
    put(keys[b*n + i], n);//以该线程在缓冲池内的值为表键，在表内放置键值对
  }
  t1 = now();
  printf("%ld: put time = %f\n", n, t1-t0);

  // Should use pthread_barrier, but MacOS doesn't support it ...
  __sync_fetch_and_add(&done, 1);//原子自加

  while (done < nthread) ;//忙等待

  t0 = now();
  //等待所有线程完成工作后调用get函数，检查miss
  for (i = 0; i < NKEYS; i++) {
    struct entry *e = get(keys[i]);
    if (e == 0) k++;
  }
  t1 = now();
  printf("%ld: get time = %f\n", n, t1-t0);
  printf("%ld: %d keys missing\n", n, k);
  return NULL;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  for(int i=0;i<NBUCKET;i++)
  pthread_mutex_init(&lock[i],NULL);

  //异常处理 当命令行参数未给出线程数时，退出报错
  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }

  nthread = atoi(argv[1]);//字符串转int

  tha = malloc(sizeof(pthread_t) * nthread);//创建线程容器(指针)，数量为nthread

  srandom(0);//种植随机数种子（推测是减少调用开销）

  assert(NKEYS % nthread == 0);//assert断言,检查总值能否被平均分给每一个线程

  //生成随机数列填充缓冲池
  for (i = 0; i < NKEYS; i++) {
    keys[i] = random();
  }

  t0 = now();//计算时间

  //生成线程
  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }

  //等待所有线程join
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }

  t1 = now();
  printf("completion time = %f\n", t1-t0);
}
