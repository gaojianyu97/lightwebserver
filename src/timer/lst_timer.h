#ifndef LST_TIMER_H
#define LST_TIMER_H

#include "../log/log.h"
#include "../httpconn/http_conn.h"

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/stat.h>

class util_timer;

/**
 * @brief 客户端数据结构
*/
struct client_data{
    sockaddr_in address; //客户端地址
    int sockfd;          //客户端套接字描述符
    util_timer *timer;   //计时器指针
};


/**
 * @brief util_timer工具类，管理计时器相关信息和操作
*/
class util_timer{
public:
    util_timer():prev(NULL),next(NULL) {}

public:
    time_t expire;                  //计时器过期时间
    void (*cb_func)(client_data *); //计时器回调函数指针
    client_data *user_data;         //用户数据
    util_timer *prev;               //前一个计时器
    util_timer *next;               //后一个计时器
};


/**
 * @brief sort_timer_lst类排序的计时器链表，用于管理多个计时器对象，并按照计时器的过期时间进行排序和触发
*/
class sort_timer_lst{
private:
    util_timer *head;//计时器链表头部
    util_timer *tail;//计时器链表尾部

    /**
     * @brief 将计时器添加到链表中
     * @param timer 计时器指针
     * @param lst_head 计时器链表头
    */
    void add_timer(util_timer *timer, util_timer *lst_head);

public:
    /**
     * @brief 构造函数，计时器链表初始化
    */
    sort_timer_lst();

    /**
     * @brief 析构函数，计时器链表资源回收
    */
    ~sort_timer_lst();
    
    /**
     * @brief 添加计时器
    */
    void add_timer(util_timer *timer);

    /**
     * @brief 调整计时器
    */
    void adjust_timer(util_timer *timer);

    /**
     * @brief 删除计时器
    */
    void del_timer(util_timer *timer);

    /**
     * @brief 计时器触发处理
    */
    void tick();
};

/**
 * @brief utils工具箱类，epoll任务处理
*/
class Utils{
public:
    static int *u_pipefd;       //管道描述符
    sort_timer_lst m_timer_lst; //排序计时器链表
    static int u_epollfd;       //epoll实例描述符
    int m_timeslot;             //最小超时单位

public:
    /**
     * @brief 构造函数，工具类初始化
    */
    Utils(){}

    /**
     * @brief 析构函数，离开作用域时进行资源回收
    */
    ~Utils(){}

    /**
     * @brief 初始化时间槽大小
     * @param timeslot 时间槽大小
    */
    void init(int timeslot);

    /**
     * @brief 设置文件描述符为非阻塞模式
     * @param fd 文件描述符
    */
    int set_nonblocking(int fd);

    /**
     * @brief 注册文件描述符到 epoll 事件表
     * @param epollfd epoll文件描述符
     * @param fd 要注册的文件描述符
     * @param one_shot 是否开启EPOLLONESHOT模式
     * @param trig_mode 触发模式
    */
    void add_fd(int epollfd, int fd, bool one_shot, int trig_mode);

    /**
     * @brief 信号处理
     * @param sig 信号
    */
    static void sig_handler(int sig);

    /**
     * @brief 设置信号处理函数
     * @param sig 要注册的信号编号
     * @param handler 信号处理函数的指针
     * @param restart 是否启用信号重启机制
    */
    void add_sig(int sig, void(handler)(int), bool restart = true);

    /**
     * @brief 定时处理任务
    */
    void timer_handler();

    /**
     * @brief 向指定的连接发送错误信息，并关闭连接
     * @param conn_fd 连接文件描述符
     * @param info 要发送的错信息字符串
    */
    void show_error(int conn_fd, const char *info);
};

/**
 * @brief 回调函数，关闭超时的客户端连接，并清理相应的资源
*/
void cb_func(client_data *user_data);

#endif