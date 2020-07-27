#include "common.h"

void handler(void *arg)				
{
	//防止死锁，所以在这里添加解锁操作
	pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void sighand(int signum)
{
	switch_run = false;
}

void reset_switch(void)
{
	switch_run = true;
}

/* 每一个线程池中的线程所执行的内容， arg就是线程池的地址 */
void *routine(void *arg)
{
	//将线程池的地址存放进去pool
	thread_pool *pool = (thread_pool *)arg;
	//定义一个缓冲指针，后期任务队列遍历的时候使用
	struct task *p;
	
	while(1)
	{
		//提前登记线程被取消后需要处理的事情	
		pthread_cleanup_push(handler, (void *)&pool->lock);
		//由于需要操作线程池中的共有资源，所以加锁
		pthread_mutex_lock(&pool->lock);

		//判断是否没有需要运行的任务
		while(pool->waiting_tasks == 0 && !pool->shutdown)
		{
			//让线程睡眠
			pthread_cond_wait(&pool->cond, &pool->lock);
		}

		//判断线程池是否没有任务并且需要关闭
		if(pool->waiting_tasks == 0 && pool->shutdown == true)
		{
			//解锁
			pthread_mutex_unlock(&pool->lock);
			//退出线程
			pthread_exit(NULL);
		}

		printf("I'm aWeak now.\n");

		//接收到SIGUSR1信号后执行动作sighand
		signal(SIGUSR1, sighand);

		//让p登记需要运行的任务节点
		p = pool->task_list->next;
		//将此任务节点从链表中删除
		pool->task_list->next = p->next;
		//将等待运行的任务队列-1
		pool->waiting_tasks -= 1;

		//解锁
		pthread_mutex_unlock(&pool->lock);
		//解除登记取消线程之后所做的函数
		pthread_cleanup_pop(0);

		//忽略线程的取消操作
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		//函数调用，进行提前设置的任务函数，参数为arg
		(p->task)(&(p->info), pool ->agent);
		//重启线程取消操作
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

		//释放任务节点
		free(p);
	}

	pthread_exit(NULL);												
}

/* pool：线程池结构体的地址 */
bool init_pool(thread_pool *pool)			
{
	
	//初始化互斥锁
	pthread_mutex_init(&pool->lock, NULL);
	//初始化条件变量
	pthread_cond_init(&pool->cond, NULL);

	//开启线程池的标志false：开	
	pool->shutdown = false;	
	//申请任务链表头				
	pool->task_list = malloc(sizeof(struct task));	
	//申请最大线程数量的ID内存		
	pool->tids = malloc(sizeof(pthread_t) * MAX_ACTIVE_THREADS);

	//判断三个申请的内存是否成功
	if(pool->task_list == NULL || pool->tids == NULL)				
	{
		perror("内存申请失败！");
		return false;
	}

	//初始化链表头，将下一个节点指向NULL
	pool->task_list->next = NULL;	
									
	//将等待运行的任务数量置0
	pool->waiting_tasks = 0;

	//将当前的线程数量置零
	pool->active_threads = 0;

	init_staff(pool);

	int i;

	for(i = 1; ;i += 1)						
	{
		//每一个线程都去跑routine这个函数的内容
		if(pthread_create(&((pool->tids)[i]), NULL,
					routine, (void *)pool) != 0)
		{
			perror("create threads error");
			return false;
		}

		//登记当前的线程数量
		pool->active_threads += 1;
		//判断链表是否到达末尾
		if (pool->agent[i].next == NULL)
			break;
	}

	return true;
}

/*
	投放任务：pool：线程池地址；
	task：任务需要运行的内容的函数指针；
	arg：传入给task函数的参数
*/
bool add_task(thread_pool *pool,
			void *(*task)(void *arg1, void *arg2), 
			void *arg)
{
	//新建一个任务节点
	struct task *new_task = malloc(sizeof(struct task));
	user_t * n = arg;
	if(new_task == NULL)
	{
		perror("内存申请失败！");
		return false;
	}
	//将任务需要做的函数存进task指针中
	new_task->task = task;
	//将任务函数参数记录在arg里面
	new_task->info = *n;
	//将任务节点的下一个位置指向NULL
	new_task->next = NULL;

	//上锁
	pthread_mutex_lock(&pool->lock);
	if(pool->waiting_tasks >= MAX_WAITING_TASKS)
	{
		//解锁
		pthread_mutex_unlock(&pool->lock);
		//反馈太多任务了
		fprintf(stderr, "任务数量太多了\n");
		//释放掉刚才登记的任务节点
		free(new_task);

		//返回添加不了任务到任务链表中
		return false;
	}
	
	//将线程池中任务链表的头节点登记到tmp
	struct task *tmp = pool->task_list;
	//将tmp指向最后的节点的位置
	while(tmp->next != NULL)
		tmp = tmp->next;

	//将新建的任务节点插入到链表中
	tmp->next = new_task;
	//将等待的任务数量+1
	pool->waiting_tasks += 1;

	//解锁
	pthread_mutex_unlock(&pool->lock);
	//唤醒正在睡眠中的线程
	pthread_cond_signal(&pool->cond);

	return true;//返回添加成功
}

/* 添加线程到线程池中 pool：线程池结构体地址， arg:添加的员工信息 */
bool add_thread(thread_pool *pool, void *arg)
{
	if(arg == NULL)
		return false;

	//新建线程
	int i = 0;
	int j = 0;
	staff_t * n = arg;
	staff_t new = * n;
	while(pool->agent[i].next != NULL)
		i += 1;
	if (i != 0)
		j = i + 1;
	else
		j = i;
	//将传输的员工信息存入链表
	pool->agent[j] = new;
	//将新节点加入链表中
	if (i != 0)
		pool->agent[i].next = &(pool->agent[j]);
	//调整新节点的指针指向
	pool->agent[j].next = NULL;

	if(pthread_create(&((pool->tids)[j]),
			NULL, routine, (void *)pool) != 0)
	{
		perror("add threads error");
	}

	//将最后成功添加到线程池的线程总数记录线程池中
	pool->active_threads += 1;
	//返回新建了多少条线程
	return true;
}

/*
	删除线程
	pool:线程池结构体地址
	arg:要删除的员工信息地址
 */
int remove_thread(thread_pool *pool, void *arg)
{
	int i;
	if(arg == NULL)
	{
		staff_t * x;
		printf("当前已经在线的工作员：\n");
		x = pool->agent[0].next;
		while(1)
		{
			printf("%s", x->name);
			if (x->next == NULL)
			{
				break;
			}
			x = x->next;
		}
		return pool->active_threads;
	}

	staff_t * n = arg;

	//开始取消指定的线程
	i = 1;
	while(strcmp(pool->agent[i].name, n->name) != 0)
	{
		i += 1;
	}
	if (pool->agent[i - 1].next != &(pool->agent[i]))
	{
		printf("工作员\n%s不存在！\n", pool->agent[i].name);
		return -1;
	}

	errno = pthread_cancel(pool->tids[i]);
	//调整链表的指向
	if (pool->agent[i].next != NULL)
	{
		/*
			当节点非头尾节点时
			链接前后节点
		 */
		pool->agent[i - 1].next = pool->agent[i].next;
	}
	else if (pool->agent[i].next == NULL)
	{
		/*
			当节点是尾节点时
			调整前节点
		 */
		pool->agent[i - 1].next = NULL;
	}
	pool->agent[i].next = NULL;

	//将新的线程数量登记active_threads
	pool->active_threads -= 1;
	
	//返回剩下多少条线程在线程池中
	return pool->active_threads;
}

bool destroy_pool(thread_pool *pool)
{
	//使能线程池的退出开关
	pool->shutdown = true;
	//将所有的线程全部唤醒
	pthread_cond_broadcast(&pool->cond);

	save_staff_info(pool);

	int i;

	//开始接合线程
	for(i = 1; ; i += 1)	
	{
		if (pool->agent[i].next == NULL)
			break;

		errno = pthread_join(pool->tids[i], NULL);
		if(errno != 0)
		{
			printf("join tids[%d] error: %s\n",
					i, strerror(errno));
		}
	}
	printf("线程结合完成！\n");

	//释放掉任务头节点
	free(pool->task_list);
	//释放掉线程ID内存
	free(pool->tids);

	return true;
}