#pragma once
#include <event2/event.h>
#include <list>
#include <mutex>
class XTask;
class XThread
{
public:
	//启动线程
	void Start();

	//线程入口函数
	void Main();

	//安装线程，初始化event_base和管道监听事件用于激活
	bool Setup();

	//线程池激活（分发）
	void Notify(evutil_socket_t fd, short which);

	//线程激活
	void Activate();

	//添加处理的任务，1个线程同时可以处理多个任务，共用一个event_base
	void AddTask(XTask* task);

	//线程编号
	int id = 0;
	XThread();
	~XThread();
private:
	int notify_send_fd = 0;
	struct event_base* base = 0;
	//任务列表
	std::list<XTask*> taskList;

	//线程安全 互斥变量
	std::mutex taskListMutex;
};

