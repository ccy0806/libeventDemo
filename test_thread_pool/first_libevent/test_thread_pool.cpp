// first_libevent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <event2/event.h>
#include <iostream>
#include <event2/listener.h>
#include "XThreadPool.h"
#include "XFtpServerCMD.h"
#define PORT 9999
using namespace std;
void listen_cb(struct evconnlistener* evl, evutil_socket_t est, struct sockaddr* addr, int socklen, void* arg)
{
    cout << "listen_cb" << endl;
    XFtpServerCMD* task = new XFtpServerCMD();
    task->sock = est;
    XThreadPool::GetInstance()->Dispatch(task);
}
int main()
{
#ifdef _WIN32
    //初始化socket库
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    //忽略管道信号，发送数据给已关闭的socket会使程序崩溃
    if (signal(SIGPIPE, SIG_IGN) == SIGERR)
        return 1;
#endif
    std::cout << "test thread_pool!\n";
    //1 初始化线程池
    XThreadPool::GetInstance()->Init(10);
    //创建libevent的上下文
    event_base *base = event_base_new();
    if (base)
    {
        cout << "event_base_new success!" << endl;
    }
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    //监听端口

    //socket 创建，bind,listen,绑定事件
    evconnlistener *ev = evconnlistener_new_bind(base,
        listen_cb,          //接收到连接的回调函数
        base,                    //回调函数获取的参数
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,   //地址重用，listen关闭同时关闭socket
        10,                 //listen函数中， 连接队列大小
        (sockaddr*)&sin,                    //绑定的地址和端口
        sizeof(sin)
        );


    //事件分发处理
    if(base)
        event_base_dispatch(base);
    if(ev)
        evconnlistener_free(ev);

    //销毁资源
    if(base)
        event_base_free(base);

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
