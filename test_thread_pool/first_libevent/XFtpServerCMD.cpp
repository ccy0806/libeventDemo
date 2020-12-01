#include "XFtpServerCMD.h"
#include <iostream>
#include <event2/bufferevent.h>
#include <event2/event.h>
using namespace std;
static void ReadCB(bufferevent *bev, void *arg)
{
	XFtpServerCMD* cmd = (XFtpServerCMD*)arg;
	char data[1024] = { 0 };
	for (;;)
	{
		int len = bufferevent_read(bev, data, sizeof(data) - 1);
		if (len <= 0)break;
		data[len] = '\0';
		cout << data << flush;
	}
}
static void EventCB(struct bufferevent* bev, short what, void* arg)
{
	XFtpServerCMD* cmd = (XFtpServerCMD*)arg;
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT" << endl; 
		bufferevent_free(bev);
		delete cmd;
		return;
	}
}
//初始化任务 运行在子线程中
bool XFtpServerCMD::Init()
{
	cout << "XFtpServerCMD::Init()\n";
	//监听socket,使用bufferevent
	bufferevent *bev = bufferevent_socket_new(this->base, this->sock, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, ReadCB, 0, EventCB, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	//添加超时
	timeval read_timeout = { 10, 0 };
	bufferevent_set_timeouts(bev, &read_timeout, 0);
	return true;
}