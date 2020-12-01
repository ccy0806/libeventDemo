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
//static ����ֻ�ڱ�cpp����Ч,���Է�ֹ����ļ�����������ͻ
//�����߳�����Ļص�����
static void NotifyCB(evutil_socket_t fd, short witch, void* arg)
{
	XThread* t = (XThread*)arg;
	t->Notify(fd, witch);
}
void XThread::Notify(evutil_socket_t fd, short which)
{
	//ˮƽ����,ֻҪû�н�����ȫ�����ٴν���
	char buf[2] = { 0 };
#ifdef _WIN32
	int recLen = recv(fd, buf, 1, 0);
#else
	int recLen = read(fd, buf, 1);
#endif
	if (recLen <= 0)
		return;
	cout << "thread id = " << id << "buffer_content: " << buf << endl;
	//ȡ������,����ʼ������
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

//�����߳�
void XThread::Start()
{
	Setup();
	//�����̣߳�
	thread th(&XThread::Main, this);
	//�Ͽ������߳���ϵ
	th.detach();
}
//�߳���ں���
void XThread::Main()
{
	cout << id << "XThread::Main() begin" << endl;
	event_base_dispatch(base);
	event_base_free(base);
	cout << id << "XThread::Main() end" << endl;
}

bool XThread::Setup()
{
	//Windows�����socket, linux �ùܵ�
#ifdef _WIN32
	//����һ��socketpair���Ի���ͨ�ţ�fds[0]����fds[1]д
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0)
	{
		cout << "evutil_socketpair failed\n";
			return false;
	}
	//���óɷ�����
	evutil_make_socket_nonblocking(fds[0]);
	evutil_make_socket_nonblocking(fds[1]);
#else
	//�ܵ�
	int fds[2];
	if (pipe(fds))
	{
		cerr << "pipe failed\n";
		return false;
	}
#endif
	//��ȡ �󶨵�event�¼��У� д��Ҫ����
	notify_send_fd = fds[1];
	//����libevent������(����)
	event_config* ev_config = event_config_new();
	event_config_set_flag(ev_config, EVENT_BASE_FLAG_NOLOCK);
	this->base = event_base_new_with_config(ev_config);
	event_config_free(ev_config);
	if (!this->base)
	{
		cerr << "event_base_new_with_config failed in thread\n";
		return false;
	}
	//��ӹܵ������¼������ڼ����߳�ִ������
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

//��Ӵ��������1���߳�ͬʱ���Դ��������񣬹���һ��event_base
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
