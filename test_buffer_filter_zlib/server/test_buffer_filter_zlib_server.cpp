// first_libevent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <event2/event.h>
#include <iostream>
#include <string.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <zlib.h>
#ifndef _WIN32
#include <signal.h>
#endif
#define PORT 9999

using namespace std;
struct Status
{
    bool start = false;
    char *filename;
    FILE *fp = 0;
    z_stream *zlib_in = 0;
    ~Status()
    {
        cout<<"析构"<<endl;
        if(fp)
            fclose(fp);
        if(zlib_in)
            inflateEnd(zlib_in);
        delete zlib_in;
        zlib_in = 0;
    }
};
bufferevent_filter_result filter_in(evbuffer *src, evbuffer *dest, ev_ssize_t limit, bufferevent_flush_mode mode, void *arg)
{
    Status * status = (Status*)arg;
	//接收客户端发送的文件名，回复OK
    if(!status->start)
    {
        char data[BUFSIZ] = {0};
        int len = evbuffer_remove(src, data, sizeof(data) - 1);
        if(len > 0)
        cout <<"server receive: "<< data << endl;
        evbuffer_add(dest, data, len);
        return BEV_OK;
    }
    //解压
    evbuffer_iovec v_in[1];
    //读取数据， 不清理缓冲
    int len = evbuffer_peek(src, -1, NULL, v_in, 1);

    if(len <= 0)
        return BEV_NEED_MORE;
    z_stream *p = status->zlib_in;
    if(!p)return BEV_ERROR;
    //zlib输入数据大小
    p->avail_in = v_in[0].iov_len;
    //zlib输入数据地址
    p->next_in = (Byte*)v_in[0].iov_base;
    //申请空间大小
    evbuffer_iovec v_out[1];
    evbuffer_reserve_space(dest, BUFSIZ, v_out, 1);

    //zlib输出空间大小
    //zlib输出空间地址
    p->avail_out = v_out[0].iov_len;
    p->next_out = (Byte*)v_out[0].iov_base;    

    int result = inflate(p, Z_SYNC_FLUSH);
    if(result != Z_OK)
    {
        cerr<<"inflate failed"<<endl;
    }
    //解压用了多少数据，从source evbuffer中移除
    //解压后数据传入dest evbuffer
    //p->avail_in未处理数据的大小  zlibedLen：解压了多少
    int zlibedLen = v_in[0].iov_len - p->avail_in;
    //p->avail_out剩余空间大小 zliboutLen：解压后大小
    int zliboutLen = v_out[0].iov_len - p->avail_out;
    //移除evbuffer数据
    cout<<"开始解压"<<endl;
    evbuffer_drain(src, zlibedLen);
    cout << "解压前："<< zlibedLen << "解压后： " << zliboutLen << endl;
    v_out[0].iov_len = zliboutLen;
    evbuffer_commit_space(dest, v_out, 1);
    cout<<"evbuffer_commit_space"<<endl;
	return BEV_OK;
}
bufferevent_filter_result filter_out(evbuffer *src, evbuffer *dest, ev_ssize_t limit, bufferevent_flush_mode mode, void *arg)
{
	return BEV_OK;
}
void read_cb(bufferevent *bev, void *arg)
{
	//接收客户端发送的文件名filter后，回复OK
    Status * status = (Status*)arg;
    cout<<"status->start = "<< status->start <<endl;
    if(!status->start)
    {
        //001 收到文件名
        char data[BUFSIZ] = {0};
        bufferevent_read(bev, data, sizeof(data) - 1);
        status->filename = data;
        string out = "out/";
        out += data;
        //打开写入文件
        status->fp = fopen(out.c_str(), "wb");
        if(!status->fp)
            cout<< "server open file failed" <<endl;
        //002回复OK
        string str = "OK";
        bufferevent_write(bev, str.c_str(), sizeof(str));
        status->start = true;
        cout<<"status->start = true"<<endl;
        return;
    }
    cout<<"---------"<<endl;
    //写入文件, 可能没读完。
    do{
        
        char data[BUFSIZ] = {0};
        int len = bufferevent_read(bev, data, sizeof(data));
        cout << "read...."<< len << endl;
        if(len >= 0)
            fwrite(data, 1, len, status->fp);
        cout<<"evbuffer_get_length(bufferevent_get_input(bev))="<<evbuffer_get_length(bufferevent_get_input(bev))<<endl;
    }while(evbuffer_get_length(bufferevent_get_input(bev)) > 0);

    cout<<"read_cb"<<endl;

}
void write_cb(bufferevent *bev, void *arg)
{
    cout << "write_cb" << endl;
}
void event_cb(bufferevent *bev, short events, void *arg)
{
    Status * status = (Status*)arg;
    cout << "server event_cb" <<endl;
    if(events & BEV_EVENT_EOF)
    {
        fclose(status->fp);
        status->fp = 0;
        delete status;
        bufferevent_free(bev);
        cout<< "BEV_EVENT_EOF" << endl;
    }
}
void listen_cb(struct evconnlistener* evl, evutil_socket_t est, struct sockaddr* addr, int socklen, void* arg)
{
    cout << "listen_cb" << endl;
    //创建bufferevent
    event_base *base = static_cast<event_base*>(arg);
    bufferevent *bev = bufferevent_socket_new(base, est, BEV_OPT_CLOSE_ON_FREE);
    //添加过滤
    Status *status = new Status();
    status->zlib_in = new z_stream();
    inflateInit(status->zlib_in);
    bufferevent *bev_filter = bufferevent_filter_new(bev, 
    	filter_in,//输入过滤
    	0,//输出过滤
    	BEV_OPT_CLOSE_ON_FREE, //关闭filter同时关闭bufferevent
    	0,//清理回调
    	status//传递参数
    	);
    //设置回调
    bufferevent_setcb(bev_filter, read_cb, write_cb, event_cb, 
    	status);//回调的参数
    bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}
int main()
{
#ifdef _WIN32
    //初始化socket库
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    //忽略管道信号，发送数据给已关闭的socket会使程序崩溃
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;
#endif
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
