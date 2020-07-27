#include "common.h"

void init_staff(thread_pool *pool)
{
	//打开staff.txt
	FILE * fp = fopen("staff.txt", "r+");

	//申请最大线程数量的staff内存
	pool->agent = malloc(sizeof(staff_t) * MAX_ACTIVE_THREADS);

	if (pool->agent == NULL)
	{
		perror("内存申请失败！");
		exit(-1);
	}

	//创建线程池里面的线程
	int i, j;
	char buf[32];
	for (i = 1; ; i += 1)
	{
		/*
			从staff.txt中一行一行地读取信息
			保存到pool->agent[i]中
			读取完成之后退出循环
		 */	
		if (i == MAX_ACTIVE_THREADS)
		{
			printf("已读取到员工的最大值！\n");
			break;
		}

		for (j = 0; ; j += 1)
		{
			bzero(buf, 32);
			if (fgets(buf, 32, fp) == NULL)
			{
				if (feof(fp))
				{
					break;
				}
				else if (ferror(fp))
				{
					perror("fgetc()");
					exit(-1);
				}
			}

			switch(j)
			{
				case 0:
					strcpy(pool->agent[i].name, buf);
					printf("姓名:%s", pool->agent[i].name);
					break;
				case 1:
					strcpy(pool->agent[i].sex, buf);
					printf("性别:%s", pool->agent[i].sex);
					break;
				case 2:
					strcpy(pool->agent[i].tel, buf);
					printf("电话:%s", pool->agent[i].tel);
					break;
				case 3:
					printf("已上线\n\n");
					break;
			}

			if (j == 3) break;
		}

		if (feof(fp))
		{
			//i记录拥有信息的节点个数
			printf("员工信息读取完成！\n\n");
			break;
		}
	}

	pool->agent[0].next = &(pool->agent[1]);
	for (j = 1; j <= i; j += 1)
	{
		/*
			调整pool->agent中的agent链表
			使它的next指针分别指向自己的下一位
			即pool->agent[i]->next = pool->agent[i + 1]
			当到达链表末尾时，使pool->agent[i]->next = NULL
			之后退出循环
		 */

		if (j < i)
			pool->agent[j].next = &(pool->agent[j + 1]);
		else
			pool->agent[j].next = NULL;
		/*
			调试用函数
		 */
		// printf("%s ->\n", pool->agent[j].name);
		// if (pool->agent[j].next == NULL)
		// {
		// 	printf(" NULL\n");
		// }
		/*
			调试用函数
		 */
	}

	//关闭文件"staff.txt"
	fclose(fp);
}

void save_staff_info(thread_pool *pool)
{
	//将储存员工信息的文件打开
	FILE * fp = fopen("staff.txt", "w+");
	char buf[32];
	int j, i = 0;

	//将员工信息写入“staff.txt”
	for (i = 1; ; i += 1)
	{
		if (i != 1)
			fputs("\n", fp);
		for (j = 0; ; j += 1)
		{
			switch(j)
			{
				case 0: 
					{
						bzero(buf, 32);
						strcpy(buf, pool->agent[i].name);
						fputs(buf, fp);
						break;
					}
				case 1: 
					{
						bzero(buf, 32);
						strcpy(buf, pool->agent[i].sex);
						fputs(buf, fp);
						break;
					}
				case 2: 
					{
						bzero(buf, 32);
						strcpy(buf, pool->agent[i].tel);
						fputs(buf, fp);
						break;
					}
				case 3:
					{
						fputs("1", fp);
						break;
					}
			}
			if (j == 3)
				break;
		}

		if (pool->agent[i].next == NULL)
			break;
	}
	printf("员工信息保存完成！\n");

	//释放掉文件指针
	fclose(fp);
}

staff_t* find_staff(thread_pool *pool)
{
	int i = 1;
	char run_man[128];

	scanf("%s", run_man);
	strcat(run_man, "\n");
	while(strcmp(pool->agent[i].name, run_man) != 0)
	{
		if (pool->agent[i].next == NULL)
		{
			printf("工作员 %s 不存在！\n", run_man);
			return NULL;
		}
		i += 1;
	}

	return &(pool->agent[i]);
}

user_t input_task(void)
{
	user_t user;

	printf("输入姓名：\n");
	scanf("%s", user.name);
	printf("输入联系电话：\n");
	scanf("%s", user.tel);
	printf("输入佣金金额：\n");
	scanf("%s", user.pay);
	printf("输入工作内容：\n");
	scanf("%s", user.user_task);
	printf("输入工作时间（秒）:\n");
	scanf("%d", &user.time);

	return user;
}

staff_t join_staff(void)
{
	staff_t new_staff;

	printf("输入姓名：\n");
	scanf("%s", new_staff.name);
	strcat(new_staff.name, "\n");
	printf("输入性别：\n");
	scanf("%s", new_staff.sex);
	strcat(new_staff.sex, "\n");
	printf("输入联系电话：\n");
	scanf("%s", new_staff.tel);
	strcat(new_staff.tel, "\n");

	return new_staff;
}

/*
	往指定的管道中发送一条消息
	fd是管道文件标识
	arg是要发送的消息
 */
void fifo_send_user(int fd, void *arg)
{
	if(fd == -1)
	{
		perror("打开管道出错\n");
		exit(-1);
	}

	ssize_t wr_ret;
	// user_t * n = arg;
	user_t * buffer= arg;

	//将数据写入管道中
	wr_ret = write(fd, buffer, sizeof(user_t));
	if(wr_ret == -1)
	{
		perror("写入管道数据异常");
	}
}

/*
	从指定的管道中接收一条消息
	fd是管道文件标识
 */
user_t fifo_rec_user(int fd)
{
	if(fd == -1)
	{
		perror("打开管道出错\n");
		exit(-1);
	}

	ssize_t rd_ret;
	user_t buffer[sizeof(user_t)];

	bzero(buffer, sizeof(buffer));

	rd_ret = read(fd, buffer, sizeof(buffer));
	if(rd_ret == -1)
	{
		perror("读取管道数据异常");
	}

	return *buffer;
}

/*
	往指定的管道中发送一条消息
	fd是管道文件标识
	arg是要发送的消息
 */
void fifo_send_text(int fd, void *arg)
{
	if(fd == -1)
	{
		perror("打开管道出错\n");
		exit(-1);
	}

	ssize_t wr_ret;
	char * n = arg;
	char buffer[256];

	strcpy(buffer, n);

	//将数据写入管道中
	wr_ret = write(fd, buffer, strlen(buffer));
	if(wr_ret == -1)
	{
		perror("写入管道数据异常");
	}
}

/*
	从指定的管道中接收一条消息
	fd是管道文件标识
 */
void fifo_rec_text(int fd, char *msg)
{
	if(fd == -1)
	{
		perror("打开管道出错\n");
		exit(-1);
	}

	ssize_t rd_ret;
	char buffer[256];

	bzero(buffer, sizeof(buffer));

	rd_ret = read(fd, buffer, sizeof(buffer));
	if(rd_ret == -1)
	{
		perror("读取管道数据异常");
	}

	msg = buffer;
}