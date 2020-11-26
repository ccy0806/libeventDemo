// first_libevent.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <event2/event.h>
#include <iostream>
#include <string.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <unistd.h>
#include <zlib.h>
#ifndef _WIN32
#include <signal.h>

#endif
#define PORT 9999
using namespace std;
struct ClientStatus
{
    FILE *fp = 0;
    z_stream *z_output = 0;
    bool startSend = false;
    bool isfpEnd = false;

    ~ClientStatus()
    {
        cout<<"析构"<<endl;
        if(z_output)
            deflateEnd(z_output);
        delete z_output;
        z_output = 0;
        cout<<"close file"<<endl;
        if(fp)
            fclose(fp);
        fp = 0;
    }
};

bufferevent_filter_result filter_in(evbuffer *src, evbuffer *dest, ev_ssize_t limit, bufferevent_flush_mode mode, void *arg)
{

	return BEV_OK;
}
bufferevent_filter_result filter_out(evbuffer *src, evbuffer *dest, ev_ssize_t limit, bufferevent_flush_mode mode, void *arg)
{
    ClientStatus *clientstatus = static_cast<ClientStatus*>(arg);
    cout << "filter_out" <<endl;
    //还没开始，接收OK后开始


    //开始压缩文件
    //取出buffer中数据的引用
    evbuffer_iovec v_in[1];
    int buffer_len = evbuffer_peek(src, -1, 0, v_in, 1);
    cout << "buffer_len = " << buffer_len << endl;
    if(buffer_len <= 0)
    {
        //没有数据
        //调用write回调，清理空间
        if(clientstatus->isfpEnd)
        {
           cout<<"文件读取结束"<<endl;
           return BEV_OK;
        }
        cout << "return BEV_NEED_MORE" <<endl;
        //return BEV_OK;
        return BEV_NEED_MORE;
    }
    z_stream *p = clientstatus->z_output;
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

    int result = deflate(p, Z_SYNC_FLUSH);
    if(result != Z_OK)
    {
        cerr<<"deflate failed"<<endl;
    }
    //压缩用了多少数据，从source evbuffer中移除
    //压缩后数据传入dest evbuffer
    //p->avail_in未处理数据的大小  zlibedLen：压缩了多少
    int zlibedLen = v_in[0].iov_len - p->avail_in;
    //p->avail_out剩余空间大小 zliboutLen：压缩后大小
    int zliboutLen = v_out[0].iov_len - p->avail_out;
    //移除evbuffer数据
    cout<<"开始压缩"<<endl;
    evbuffer_drain(src, zlibedLen);
    cout << "压缩前："<< zlibedLen << "压缩后： " << zliboutLen << endl;
    v_out[0].iov_len = zliboutLen;
    evbuffer_commit_space(dest, v_out, 1);
    cout<<"evbuffer_commit_space"<<endl;


/*
    char data[BUFSIZ] = {0};
    int len = evbuffer_remove(src, data, sizeof(data) - 1);
    cout<<"len = "<<len<<endl;
    evbuffer_add(dest, data, len);
*/
    return BEV_OK;
}
void read_cb(bufferevent *bev, void *arg)
{
    ClientStatus *clientstatus = static_cast<ClientStatus*>(arg);
    // 002接收服务端发送的OK
    char data[BUFSIZ] = {0};
    int len = bufferevent_read(bev, data, sizeof(data) - 1);
    if(strcmp(data, "OK") == 0)
    {
        cout<<"startSend = ok"<<endl;
        clientstatus->startSend  = true;
        cout << data <<endl;
        //开始发送文件,触发写入回调
        bufferevent_trigger(bev, EV_WRITE, 0);
    }    

}
void write_cb(bufferevent *bev, void *arg)
{
    cout << "write_cb" << endl;
    ClientStatus *clientstatus = static_cast<ClientStatus*>(arg);
    FILE *fp = clientstatus->fp;
    if(!fp)return;
    //读取文件
    char data[BUFSIZ] = {0};
    int len = fread(data, 1, sizeof(data), fp);
    cout << "read data len:" << len << endl;
    if(len <= 0)
    {
        clientstatus->isfpEnd = true;
        cout<<"clientstatus->isfpEnd = true"<<endl;
        //判断缓冲是否有数据，如果有就刷新
        //获取过滤器绑定的buffer
        bufferevent *be = bufferevent_get_underlying(bev);
        //获取输出缓冲及其大小
        evbuffer *out_evb = bufferevent_get_output(be);
        int buflen = evbuffer_get_length(out_evb);
        cout << "evbuffer_get_length: " << buflen <<endl;
        if(buflen > 0)
        {
            cout<<"buflen > 0" <<endl;
            //有数据，刷新过滤器缓冲，不刷新缓冲，不会再次进入该函数
            bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
            cout<<"flush end"<<endl;
            //sleep(3);
            return;
        }
        if(clientstatus->isfpEnd)
        {
            delete clientstatus;
            bufferevent_free(bev);
            return;
        }    
    }
    cout<<"write_data------"<<endl;
    bufferevent_write(bev, data, len);
    
}
void event_cb(bufferevent *bev, short events, void *arg)
{

}

void client_event_cb(bufferevent *bev, short events, void *arg)
{
    cout << "client_event_cb" << events << endl;
    if(events & BEV_EVENT_CONNECTED)
    {
        //001发送文件名
        cout << "BEV_EVENT_CONNECTED" << endl;
        string filePath = "file1.txt";
        bufferevent_write(bev, filePath.c_str(), sizeof(filePath));

        //设置读取写入和事件回调
        FILE *fp = fopen(filePath.c_str(), "rb");
        //初始化zlib上下文，默认压缩方式
        ClientStatus *clientstatus = new ClientStatus();
        clientstatus->z_output = new z_stream();

        deflateInit(clientstatus->z_output, Z_DEFAULT_COMPRESSION);
        if(!fp)
        {
            cout << "open failed" << endl;
        }
        clientstatus->fp = fp;
        //创建输出过滤

        bufferevent *bevfilter = bufferevent_filter_new(bev, 
            0, //输入过滤
            filter_out, //输出过滤
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS,
            0,//请理回调
            clientstatus);//参数

        bufferevent_enable(bevfilter, EV_READ | EV_WRITE);
        bufferevent_setcb(bevfilter, read_cb, write_cb, event_cb, clientstatus) ;
        
    }
    else
    {
        cout << "other" << endl;
    }

}
int main(int argc, char *argv[])
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
    std::cout << "test libevent!\n";
    //创建libevent的上下文
    event_base *base = event_base_new();
    if (base)
    {
        cout << "event_base_new success!" << endl;
    }
    const char *ip = argv[1];
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);
    evutil_inet_pton(AF_INET, ip, &sin.sin_addr.s_addr);

    bufferevent *bev = bufferevent_socket_new(base, -1 ,BEV_OPT_CLOSE_ON_FREE);
    //只绑定事件回调，用来确认连接成功
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    bufferevent_setcb(bev, 0, 0, client_event_cb, 0);
    //建立连接
    bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));


    //事件分发处理
    if(base)
        event_base_dispatch(base);
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
