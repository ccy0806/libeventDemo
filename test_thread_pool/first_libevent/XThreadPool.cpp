#include "XThreadPool.h"
#include "XThread.h"
#include "XTask.h"
#include <thread>
#include <iostream>

using namespace std;
//分发线程
void XThreadPool::Dispatch(XTask* task)
{
	//轮询
	if (!task)return;
	int tid = (lastThread + 1) % threadCount;
	lastThread = tid;
	XThread* t = threads[tid];

	//添加任务
	t->AddTask(task);
	//激活线程
	t->Activate();
	
}

//初始化并启动线程
void XThreadPool::Init(int threadCount)
{
	this->threadCount = threadCount;
	this->lastThread = -1;
	for (int i = 0; i < threadCount; ++i)
	{
		//循环创建线程
		XThread* t = new XThread();
		t->id = i + 1;
		cout << "create thread " << i << endl;
		t->Start();
		this->threads.push_back(t);
		this_thread::sleep_for(10ms);
	}
}