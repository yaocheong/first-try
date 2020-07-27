#include "common.h"

void *mytask(void *arg1, void *arg2)
{
	//从参数中提取数据
	user_t * n = arg1;
	user_t i = *n;
	int j = i.time;
	staff_t * m = arg2;
	staff_t agent = *m;

	//打开user.txt准备记录交易记录
	FILE * fp = fopen("user.txt", "a+");
	int SIZE = sizeof(user_t) + 5;
	char * data = malloc(SIZE);

	char buf[1024];

	//打开管道fifo_1
	int fd_1;
	fd_1 = open(FIFO_1, O_RDWR);
	if(fd_1 == -1)
	{
		perror("打开管道1出错\n");
		return NULL;
	}

	//使用fifo_1传输字符串到用户端
	strcat(buf, "\n\n尊敬的客户 ");
	strcat(buf, i.name);
	strcat(buf, " 您好\n您下单的任务 ");
	strcat(buf, i.user_task);
	strcat(buf, " 已由工作员\n");
	strcat(buf, agent.name);
	strcat(buf, "领取，联系工作员请拨打\n");
	strcat(buf, agent.tel);
	strcat(buf, "\n");

	fifo_send_text(fd_1, buf);

	//显示消息到服务端
	printf("\n\n客户 %s 下单的任务 %s 已由工作员\n%s领取", 
		i.name, 
		i.user_task, 
		agent.name);

	//开始任务计时
	while(j)
	{
		sleep(1);
		j -= 1;
		//当输入“finish“时任务结束
	}

	//加班费计算为额外时间/10，向上取整
	if (j < 0)
	{
		j = abs(j);
		if (j % 10 > 0)
		{
			j = j / 4;
			j += 1;
		}
		else
			j = j / 4;
	}
	else
		j = 0;

	//使用fifo_1传输字符串到用户端
	strcat(buf, "\n\n任务 ");
	strcat(buf, i.user_task);
	strcat(buf, " 已由工作员\n");
	strcat(buf, agent.name);
	strcat(buf, "完成\n本次的任务金额为 \n");
	strcat(buf, i.pay + j);
	strcat(buf, " \n\n");

	fifo_send_text(fd_1, buf);

	//显示消息到服务端
	printf("\n\n任务 %s 已由工作员\n%s完成\n本次的任务金额为 %s \n\n", 
		i.user_task, 
		agent.name, 
		i.pay + j);

	//保存交易记录到fp
	snprintf(data, SIZE, 
		"%s\t%s\t%s\t%s\t%d\t%s\n", 
		i.name, 
		i.tel, 
		i.pay, 
		i.user_task, 
		i.time, 
		i.pay + j);
	fputs(data, fp);

	//复位动作开关
	reset_switch();

	fclose(fp);

	return NULL;
}

int main(int argc, char const *argv[])
{
	printf("正在初始化...\n");
	//根据输入的关键字切换模式
	char const * mode = argv[1];

	pid_t pid;
	char input[32];
	thread_pool *pool = malloc(sizeof(thread_pool));

	printf("*****欢迎使用ELSM任务发布平台*****\n");
	if (strcmp(mode, "work-mode") == 0)
	{
		printf("\t已进入工作模式\n");
		printf("正在加载中...\n\n");
		//自动打开一个进入客户模式的窗口
		// system("gnome-terminal -e ./ELSM user-mode");
		// ↑ The child process was aborted by signal 11.
		system("gnome-terminal");
		//重置任务计时开关
		reset_switch();
	
		int i = 0;
		staff_t new_staff;
		user_t take;

		//建立线程池
		init_pool(pool);

		//打开管道
		int fd_2;

		if(access(FIFO_2, F_OK) != 0)
		{
			mkfifo(FIFO_2, 0644);
		}

		fd_2 = open(FIFO_2, O_RDWR | O_CREAT);
		if(fd_2 == -1)
		{
			perror("打开管道2出错\n");
			return -1;
		}

		printf("\n***********系统加载完成***********\n");
		while(1)
		{
			printf("输入“join”加入新工作员\n");
			printf("输入“run”进入等待\n");
			scanf("%s", input);
			if (strcmp(input, "join") == 0)
			{
				new_staff = join_staff();

				if (add_thread(pool, &new_staff))
					printf("工作员\n%s加入已完成，请等待分配任务\n\n", 
						new_staff.name);
				else
				{
					printf("工作员录入失败，请检查系统！\n");
					return -1;
				}
			}
			else if (strcmp(input, "run") == 0)
			{
				break;
			}
		}

		pid = fork();
		//子进程专注于从管道中读取消息并转发到线程池中
		if (pid == 0)
		{
			while(1)
			{
				printf("\n正在等待接收订单\n\n");
				take = fifo_rec_user(fd_2);
				printf("\n******有新订单******\n");
				printf("委托人:%s\n", take.name);
				printf("委托人电话:%s\n", take.tel);
				printf("佣金:%s\n", take.pay);
				printf("委托内容:%s\n", take.user_task);
				printf("委托时间:%d\n", take.time);
				
				add_task(pool, mytask, &take);
				printf("1\n");
			}
		}
		else
		{
			while(1)
			{
				printf("\n输入“finish”完成当前任务\n");
				printf("输入“exit”退出系统\n");
				printf("输入“all”查看当前有多少位工作员在线\n");
				printf("输入“stop”员工请假\n");
				scanf("%s", input);
				if (strcmp(input, "all") == 0)
				{
					printf("当前共有 %d 位工作人员在线。\n", 
						remove_thread(pool, NULL));
				}
				else if (strcmp(input, "finish") == 0)
				{
					/*
						寻找指定的工作员，发送完成信号
					 */
					staff_t * man;
					printf("请输入任务确认已完成的工作员姓名：\n");
					man = find_staff(pool);
					while(strcmp(pool->agent[i].name, man->name) != 0)
						i += 1;
					pthread_kill(pool->tids[i], SIGUSR1);
				}
				else if (strcmp(input, "stop") == 0)
				{
					printf("请输入确认请假的工作员姓名：\n");
					if (remove_thread(pool, find_staff(pool)) != -1)
						printf("该工作员已暂时离开岗位\n");
				}
				else if (strcmp(input, "exit") == 0)
				{
					printf("正在退出系统...\n");
					break;
				}
			}
		}
	}
	else if (strcmp(mode, "user-mode") == 0)
	{
		printf("\t已进入客户模式\n");
		user_t user;

		//打开管道
		int fd_1;

		if(access( FIFO_1, F_OK) != 0)
		{
			mkfifo(FIFO_1, 0644);
		}

		fd_1 = open(FIFO_1, O_RDWR | O_CREAT);
		if(fd_1 == -1)
		{
			perror("打开管道1出错\n");
			return -1;
		}

		int fd_2;

		if(access( FIFO_2, F_OK) != 0)
		{
			mkfifo(FIFO_2, 0644);
		}

		fd_2 = open(FIFO_2, O_RDWR);
		if(fd_2 == -1)
		{
			perror("打开管道2出错\n");
			return -1;
		}

		//投放任务
		while(1)
		{
			printf("\n输入“run”进入下单系统\n");
			printf("输入“exit”退出系统\n");
			scanf("%s", input);
			if (strcmp(input, "run") == 0)
			{
				user = input_task();
				fifo_send_user(fd_2, &user);
				printf("下单已完成，正在等候工作人员为您完成。\n\n");

				pid = fork();
				if (pid == 0)
				{
					int i;
					char msg[256];

					for (i = 0; i < 2; i += 1)
					{
						printf("正在等待订单完成\n");
						fifo_rec_text(fd_1, msg);
						printf("%s\n", msg);
					}
				}
			}
			else if (strcmp(input, "exit") == 0)
			{
				break;
			}
		}
	}

	printf("正在关闭程序...\n");
	
	//工作模式结束处理
	if (strcmp(mode, "work-mode") == 0)
	{
		//关闭线程池
		destroy_pool(pool);
	}

	free(pool);

	printf("************系统已关闭************\n");
	
	return 0;
}