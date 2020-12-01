#pragma once
#include <vector>
//.h文件中少引用文件,不加命名空间
class XThread;
class XTask;
class XThreadPool
{
public:
	//单例模式
	static XThreadPool* GetInstance()
	{
		static XThreadPool instance;
		return &instance;
	}
	//初始化所有线程，并启动线程
	void Init(int threadCount);

	//分发线程
	void Dispatch(XTask* task);
private:
	XThreadPool() {};
	//线程数量
	int threadCount = 0;
	int lastThread = -1;
	//线程池线程
	std::vector<XThread*> threads;
};

