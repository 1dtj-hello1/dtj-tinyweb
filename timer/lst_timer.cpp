#include "lst_timer.h"
#include "../http/http_conn.h"
#include <iostream>
#include <vector>
#include <time.h>
#include <algorithm>
void sort_timer_lst::add_timer(util_timer *timer)
{
    heap.push_back(timer);
    std::push_heap(heap.begin(), heap.end(), std::greater<util_timer*>());
}
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    std::make_heap(heap.begin(), heap.end(), std::greater<util_timer*>());
}
void sort_timer_lst::del_timer(util_timer *timer)
{
    auto it = std::find(heap.begin(), heap.end(), timer);
    if (it != heap.end())
    {
        heap.erase(it);
        std::make_heap(heap.begin(), heap.end(), std::greater<util_timer*>());
    }  
}
void sort_timer_lst::tick()
{
    time_t cur = time(NULL);
    while (!heap.empty())
    {
        util_timer* top = heap.front();
        if (top->expire < cur)
            break;
        top->cb_func(top->user_data);
        std::pop_heap(heap.begin(), heap.end(), std::greater<util_timer*>());
        heap.pop_back();
        delete top;
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig)
{
    //为保证函数的可重入性，保留原来的errno
    int save_errno = errno;
    int msg = sig;
    send(u_pipefd[1], (char *)&msg, 1, 0);
    errno = save_errno;
}

//设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_lst.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn::m_user_count--;
}
