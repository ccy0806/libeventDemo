#pragma once
class XTask
{
public:
	struct event_base* base = 0;
	int sock = 0;
	int thread_id = -1;
	//��ʼ������
	virtual bool Init() = 0;
};

