#include "XThread.h"
#include "XTask.h"
#include <thread>
#include <iostream>
#include <event2/event.h>
#ifdef _WIN32
#else
#include <unistd.h>
#endif
using namespace std;
//static 函数只在本cpp内有效,可以防止别的文件函数命名冲突
//激活线程任务的回调函数
static void NotifyCB(evutil_socket_t fd, short witch, void* arg)
{
	XThread* t = (XThread*)arg;
	t->Notify(fd, witch);
}
void XThread::Notify(evutil_socket_t fd, short which)
{
	//水平触发,只要没有接收完全，会再次进来
	char buf[2] = { 0 };
#ifdef _WIN32
	int recLen = recv(fd, buf, 1, 0);
#else
	int recLen = read(fd, buf, 1);
#endif
	if (recLen <= 0)
		return;
	cout << "thread id = " << id << "buffer_content: " << buf << endl;
	//取出任务,并初始化任务
	XTask* task = NULL;
	taskListMutex.lock();
	if (taskList.empty())
	{
		taskListMutex.unlock();
		return;
	}
	task = taskList.front();
	taskList.pop_front();
	taskListMutex.unlock();
	task->Init();
}

//启动线程
void XThread::Start()
{
	Setup();
	//启动线程，
	thread th(&XThread::Main, this);
	//断开与主线程联系
	th.detach();
}
//线程入口函数
void XThread::Main()
{
	cout << id << "XThread::Main() begin" << endl;
	event_base_dispatch(base);
	event_base_free(base);
	cout << id << "XThread::Main() end" << endl;
}

bool XThread::Setup()
{
	//Windows用配对socket, linux 用管道
#ifdef _WIN32
	//创建一个socketpair可以互相通信，fds[0]读，fds[1]写
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
	{
		cout << "evutil_socketpair failed\n";
			return false;
	}
	//设置成非阻塞
	evutil_make_socket_nonblocking(fds[0]);
	evutil_make_socket_nonblocking(fds[1]);
#else
	//管道
	int fds[2];
	if (pipe(fds))
	{
		cerr << "pipe failed\n";
		return false;
	}
#endif
	//读取 绑定到event事件中， 写入要保存
	notify_send_fd = fds[1];
	//创建libevent上下文(无锁)
	event_config* ev_config = event_config_new();
	event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
	this->base = event_base_new_with_config(ev_config);
	event_config_free(ev_config);
	if (!this->base)
	{
		cerr << "event_base_new_with_config failed in thread\n";
		return false;
	}
	//添加管道监听事件，用于激活线程执行任务
	event* ev = event_new(this->base, fds[0], EV_READ | EV_PERSIST, NotifyCB, this);
	event_add(ev, 0);
	return true;
}
void XThread::Activate()
{
#ifdef _WIN32
	int res = send(this->notify_send_fd, "c", 1, 0);
#else
	int res = write(this->notify_send_fd, "c", 1);

#endif
	if (res <= 0)
	{
		cerr << "XThread::Activate failed\n";
	}
}

//添加处理的任务，1个线程同时可以处理多个任务，共用一个event_base
void XThread::AddTask(XTask* task)
{
	if (!task)return;
	task->base = this->base;
	taskListMutex.lock();
	taskList.push_back(task);
	taskListMutex.unlock();
}

XThread::XThread()
{

}
XThread::~XThread()
{

}
