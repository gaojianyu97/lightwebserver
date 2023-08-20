#include "lst_timer.h"

sort_timer_lst::sort_timer_lst(){
    head = NULL;
    tail = NULL;
}

sort_timer_lst::~sort_timer_lst(){
    util_timer *tmp = head;
    while(tmp){
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer,util_timer *lst_head){
    util_timer *prev = lst_head;//链表头
    util_timer *tmp = prev->next;

    //按照过期时间从小到大的顺序将计时器插入双向链表
    while(tmp){
        if(timer->expire<tmp->expire){
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
        prev = tmp;
        tmp = tmp->next;
    }

    //链表仅有一个头，且新进入计时器过期时间大于头节点（参考add_timer(util_timer *timer)）
    if(!tmp){
        prev->next = timer;
        timer->next = NULL;
        timer->prev = prev;
        tail = timer;
    }
}

void sort_timer_lst::add_timer(util_timer *timer){
    //timer为空代表不添加
    if(!timer)
        return;
    
    //链表为空，新加入计时器作为头尾
    if(!head){
        head = tail = timer;
        return;
    }

    //计时器过期时间小于头部，将计时器作为链表的头
    if(timer->expire<head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    add_timer(timer, head);
}

void sort_timer_lst::adjust_timer(util_timer *timer){
    // 如果 timer 为 NULL，直接返回，不进行任何操作
    if(!timer)
        return;

    util_timer *tmp = timer->next;
    // 如果 timer 的下一个计时器为 NULL 或者 timer 的过期时间小于下一个计时器的过期时间，直接返回
    if(!tmp||timer->expire<tmp->expire)
        return;
    
    // 如果 timer 是链表头部的计时器
    if(timer==head){
        head = head->next;// 更新头指针，将头指针指向 timer 的下一个计时器
        head->prev = NULL;// 更新新的头指针的前驱为 NULL
        timer->next = NULL;// 将 timer 的 next 指针置为 NULL，因为它将成为新的链表的最后一个节点
        add_timer(timer, head);// 将 timer 插入到新的链表中
    }else{
         // 如果 timer 不是链表头部的计时器
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::del_timer(util_timer *timer){
    //要删除的计时器为空
    if(!timer)
        return;
    //要删除的计时器为头和尾
    if(timer==head && timer==tail){
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }

    //要删除的计时器为头
    if(timer==head){
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }

    //要删除的计时器为尾
    if(timer==tail){
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }

    //正常删除
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    return;
}

void sort_timer_lst::tick(){
    if(!head)
        return;

    time_t cur = time(NULL);
    util_timer *tmp = head;
    //循环对头计时器进行判定
    while(tmp){
        if(cur<tmp->expire)break;

        tmp->cb_func(tmp->user_data);
        head = tmp->next;
        if(head)
            head->prev = NULL;
        delete tmp;
        tmp = head;
    }
}

void Utils::init(int timeslot){
    m_timeslot = timeslot;//触发事件时间初始化
}

int Utils::set_nonblocking(int fd){
    
    int old_option = fcntl(fd, F_GETFL);// 获取文件描述符的旧的文件状态标志
    int new_option = old_option | O_NONBLOCK;// 将旧的文件状态标志与 O_NONBLOCK 进行按位或操作，设置为非阻塞模式
    fcntl(fd, F_SETFL, new_option);// 将新的文件状态标志设置为文件描述符的状态
    return old_option;// 返回旧的文件状态标志，以便在需要时进行恢复
}

void Utils::add_fd(int epoll_fd,int fd,bool one_shot,int trig_mode){
    epoll_event event;
    event.data.fd = fd;

    // 根据传入的触发模式设置事件类型
    if(trig_mode==1){
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;//监听可读事件（有数据可读），使用边缘触发模式，并且可以检测连接的对端关闭
    }else{
        event.events = EPOLLIN | EPOLLRDHUP;//监听可读事件（有数据可读），并且可以检测连接的对端关闭
    }

    // 如果需要单次触发，则添加 EPOLLONESHOT 标志（事件只触发一次后即从epoll示例的就绪队列移除）
    if(one_shot){
        event.events |= EPOLLONESHOT;
    }

    // 将事件添加到 epoll 监听中
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);

    // 设置文件描述符为非阻塞模式
    set_nonblocking(fd);
}

void Utils::sig_handler(int sig){
    int save_error = errno;// 保存当前的 errno 值
    int msg = sig;// 将收到的信号值存储到 msg 变量中
    send(u_pipefd[1], (char *)&msg, 1, 0);// 向管道的写端发送信号值
    errno = save_error;// 向管道的写端发送信号值
}

void Utils::add_sig(int sig,void(handler)(int),bool restart){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;// 设置信号处理函数

    // 如果需要重启系统调用，则设置 SA_RESTART 标志
    if(restart){
        sa.sa_flags |= SA_RESTART;
    }
    // 如果需要重启系统调用，则设置 SA_RESTART 标志
    sigfillset(&sa.sa_mask);

    // 使用 sigaction 函数设置信号处理动作
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::timer_handler(){
    m_timer_lst.tick();// 触发定时器链表中已到期的计时器的回调函数
    alarm(m_timeslot);// 重新设置定时器，以便下一次触发
}

void Utils::show_error(int conn_fd,const char *info){
    send(conn_fd, info, strlen(info), 0);// 发送错误信息到连接套接字
    close(conn_fd);// 关闭连接套接字
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;

// 回调函数，用于处理超时的客户连接
void cb_func(client_data *user_data){
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
