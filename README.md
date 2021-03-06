<h4>
本程序基于 Linux 下的 C 语言进行
</h4>

* main.c 		包含运行中判定的函数和用户任务函数
* thread_pool.c 包含用户任务链表和线程控制函数
* other.c 		包含输入、输出数据的函数
* staff.txt 	包含保存的员工数据
* user.txt 		包含保存的用户数据
* Makefile 		Make脚本

<h4>
基于线程池的人员分配系统
</h4>

	指令：
		work-mode	进入服务端模式
		user-mode	进入用户模式

** 注意 **
	
	此时版本在work-mode中的子进程会导致阻塞，使得系统无法正常使用

	
应用场景：

	普通用户级别的需求：
		普通的用户他们有跑腿的需求，需要到指定的终端上发布他的需求：
	需求包含了一些内容：
		1，用户的名字
		2，用户的电话
		3，用户愿意出的佣金是多少
		4，用户让你干的事情是什么
		5，用户准备让你干多长的时间
	需求确定完后用户就发布信息到我们指定的系统中，等待接单，由系统自动指派跑腿人员来跟进这件事情。

	跑腿人员级别的需求：
		1，我们需要跑腿人员能够注册信息到系统当中，并且启动接单服务
	跑腿人员注册信息当中应当包含以下信息：
		1，跑腿人员的名字
		2，跑腿人员的性别
		3，跑腿人员的电话
		2，可以输入完成订单

系统需求：

	能够自动识别用户所下的单，并且自动分派跑腿人员进行跟进，并且提供以下服务：
		能够自动计时，当跑腿人员在规定时间当中无法完成任务时，我们需要另外计费（超时加班费）

技术角度分析：

	1，我们的程序应该包含普通用户的下单功能，形成一条用户的任务链表
	
	2，给跑腿人员提供注册功能（系统启动的时候自动加载跑腿人员信息文件，形成对应的线程等待工作）
	
	3，系统应单有计时功能，并且能够知道时间是否超出了用户所需要的时间
	
	4，跑腿人员有输入任务结束的功能，然后跑腿人员计算收入（包含加班费），在任务结束的时候显示出来
	
	5，系统退出功能
