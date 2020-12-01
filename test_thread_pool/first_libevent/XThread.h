#pragma once
#include <event2/event.h>
#include <list>
#include <mutex>
class XTask;
class XThread
{
public:
	//�����߳�
	void Start();

	//�߳���ں���
	void Main();

	//��װ�̣߳���ʼ��event_base�͹ܵ������¼����ڼ���
	bool Setup();

	//�̳߳ؼ���ַ���
	void Notify(evutil_socket_t fd, short which);

	//�̼߳���
	void Activate();

	//��Ӵ��������1���߳�ͬʱ���Դ��������񣬹���һ��event_base
	void AddTask(XTask* task);

	//�̱߳��
	int id = 0;
	XThread();
	~XThread();
private:
	int notify_send_fd = 0;
	struct event_base* base = 0;
	//�����б�
	std::list<XTask*> taskList;

	//�̰߳�ȫ �������
	std::mutex taskListMutex;
};

