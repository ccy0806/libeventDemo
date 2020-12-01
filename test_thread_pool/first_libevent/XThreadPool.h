#pragma once
#include <vector>
//.h�ļ����������ļ�,���������ռ�
class XThread;
class XTask;
class XThreadPool
{
public:
	//����ģʽ
	static XThreadPool* GetInstance()
	{
		static XThreadPool instance;
		return &instance;
	}
	//��ʼ�������̣߳��������߳�
	void Init(int threadCount);

	//�ַ��߳�
	void Dispatch(XTask* task);
private:
	XThreadPool() {};
	//�߳�����
	int threadCount = 0;
	int lastThread = -1;
	//�̳߳��߳�
	std::vector<XThread*> threads;
};

