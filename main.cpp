#include <arpa/inet.h>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <signal.h>

#include "HttpConn.h"
#include "Locker.h"
#include "ThreadPool.cpp"

#define MAX_FD 65536
#define MAX_EVENT_NUMBER 10000

extern void addfd(int epollfd, int fd, bool one_shot);
extern void removefd(int epollfd, int fd);

void addsig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const char *info) {
    printf("%s", info);
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int main(int argc, char *argv[]) {

    addsig(SIGPIPE, SIG_IGN);

    ThreadPool<HttpConn> *pool =NULL;
    printf("create pool Done \n");
    try {
        pool = new ThreadPool<HttpConn>;
    } catch (...) {
        return 1;
    }

    HttpConn *users = new HttpConn[MAX_FD];
    assert(users);
    int user_count = 0;

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    struct linger tmp = {1, 0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port=htons(8888);
    address.sin_addr.s_addr=0;

    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);

    ret = listen(listenfd, 5);
    assert(ret >= 0);

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    HttpConn::m_epollfd = epollfd;

    while (true) {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((number < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address,
                                    &client_addrlength);
                if (connfd < 0) {
                    printf("errno is: %d\n", errno);
                    continue;
                }

                if (HttpConn::m_user_count >= MAX_FD) {
                    show_error(connfd, "Internal server busy");
                    continue;
                }

                users[connfd].init(connfd, client_address);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sockfd].close_conn();
            } else if (events[i].events & EPOLLIN) {
                if (users[sockfd].read()) {
                    pool->append(users + sockfd);
                } else {
                    users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT) {
                if (!users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            } else {
            }
        }
    }

    close(epollfd);
    close(listenfd);
    delete[] users;
    delete pool;
    return 0;
}
