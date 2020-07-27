#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_1 "/tmp/fifo_1"	//有名管道1本地文件
#define FIFO_2 "/tmp/fifo_2"	//有名管道2本地文件

#define MAX_WAITING_TASKS	1000//最大的等待任务数量
#define MAX_ACTIVE_THREADS	20+1//最大的线程数量 + 链表头结点

bool switch_run;

typedef struct user 			//用户信息结构体
{
	char name[32]; 				//姓名
	char tel[16];				//电话
	char pay[16];				//佣金
	char user_task[1024];		//任务
	int time; 					//时间
}user_t;

typedef struct staff 			//员工信息结构体
{
	char name[32];				//姓名
	char sex[10];				//性别
	char tel[16];				//电话

	struct staff *next; 		//链表的位置结构体指针
}staff_t;

struct task						//任务链表结构体
{
	void *(*task)(void *arg1, void *arg2);	
								//任务做什么事情的函数指针
	user_t info;				//用户信息

	struct task *next;			//链表的位置结构体指针
};

typedef struct thread_pool 		//线程池结构体
{
	pthread_mutex_t lock;		//用于线程池同步互斥的互斥锁
	pthread_cond_t  cond;		//用于让线程池里面的线程睡眠的条件变量
	struct task *task_list;		//线程池的执行任务链表

	pthread_t *tids;			//线程池里面线程的ID登记处
	struct staff *agent;		//线程池中对应线程的员工信息登记处

	unsigned waiting_tasks;		//等待的任务数量，也就是上面任务链表的长度
	unsigned active_threads;	//当前已经创建的线程数

	bool shutdown;				//线程池的开关
}thread_pool;

//初始化线程池
bool
init_pool(thread_pool *pool);

//初始化员工信息链表
void init_staff(thread_pool *pool);

//往线程池里面添加任务节点
bool
add_task(thread_pool *pool,
         void *(*task)(void *arg1, void *arg2),
         void *arg);

//添加线程池中线程的数量
bool 
add_thread(thread_pool *pool,
           void *arg);

//移除线程
int 
remove_thread(thread_pool *pool,
              void *arg);

//销毁线程池
bool destroy_pool(thread_pool *pool);

//保存员工信息到文件中
void save_staff_info(thread_pool *pool);

//确认员工是否存在
staff_t* find_staff(thread_pool *pool);

//线程执行函数收到SIGUSR1时的动作
void sighand(int signum);
//线程池里面线程的执行函数
void *routine(void *arg);
//重置开关的函数
void reset_switch(void);

//创建新用户信息结构体
user_t input_task(void);
//创建新员工信息结构体
staff_t join_staff(void);

//使用管道发送用户信息
void fifo_send_user(int fd, void *arg);
//使用管道接收用户信息
user_t fifo_rec_user(int fd);
//使用管道发送文本信息
void fifo_send_text(int fd, void *arg);
//使用管道接收文本户信息
void fifo_rec_text(int fd, char *msg);

#endif