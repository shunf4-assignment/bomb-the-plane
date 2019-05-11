#include "head.h"

Application::Application(uint32_t port) {
	listenfd = call(socket, "socket", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int val = call(fcntl, "fcntl", listenfd, F_GETFL, 0);
	call(fcntl, "fcntl", listenfd, F_SETFL, val | O_NONBLOCK);
	int val1 = 1;
	call(setsockopt, "setsockopt", listenfd, SOL_SOCKET, SO_REUSEADDR, &val1, sizeof(int));
	sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	call(bind, "bind", listenfd, reinterpret_cast<const sockaddr *>(&servaddr), sizeof(servaddr));
	call(listen, "listen", listenfd, BACKLOG);

	epollfd = call(epoll_create1, "epoll_create1", 0);
	epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = listenfd;
	call(epoll_ctl, "epoll_ctl", epollfd, EPOLL_CTL_ADD, listenfd, &ev);
}

void Application::acceptConn() {
	sockaddr_in cliaddr;
	socklen_t len = sizeof(cliaddr);
	int connfd = accept(listenfd, reinterpret_cast<sockaddr *>(&cliaddr), &len);
	if (connfd == -1) {
		int errsv = errno;
		if (errsv == EWOULDBLOCK || errsv == ECONNABORTED || errsv == EPROTO || errsv == EINTR) {
			return;
		}
		else {
			perror("accept");
			std::exit(EXIT_FAILURE);
		}
	}
	char buff[16];
	std::clog << "connection from " << inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff)) << ':' << ntohs(cliaddr.sin_port) << std::endl;

	int val = call(fcntl, "fcntl", connfd, F_GETFL, 0);
	call(fcntl, "fcntl", connfd, F_SETFL, val | O_NONBLOCK);

	epoll_event ev;
	ev.data.fd = connfd;
	ev.events = EPOLLIN;
	call(epoll_ctl, "epoll_ctl", epollfd, EPOLL_CTL_ADD, connfd, &ev);

	connections.emplace(connfd, connfd);
}

void Application::run() {
		for (;;) {
			int nfds = call(epoll_wait, "epoll_wait", epollfd, events, MAX_EVENTS, -1);
			for (int i = 0; i < nfds; ++i) {
				if (events[i].data.fd == listenfd) {
					acceptConn();
				}
				else if (events[i].events & EPOLLIN) {
					auto &conn = connections.at(events[i].data.fd);
					int res = conn.input();
					if (res == 0) {
						handleDisconnection(epollfd, events[i].data.fd, connections, users);
					}
					else if (res > 0) {
						Msg msg = conn.getMsg();
						while (msg.type != MsgType::Null) {
							auto handler = Handler::factory(epollfd, events[i].data.fd, msg, connections, users);
							handler->process();
							msg = conn.getMsg();
						}
					}
				}
				else if (events[i].events & EPOLLOUT) {
					int fd = events[i].data.fd;
					auto &conn = connections.at(fd);
					if (!conn.obuf.empty()) {
						int res = conn.output();
						if (res > 0) {
							epoll_event ev;
							ev.data.fd = fd;
							if (conn.obuf.empty())
								ev.events = EPOLLIN;
							else
								ev.events = EPOLLIN | EPOLLOUT;
							call(epoll_ctl, "epoll_ctl", epollfd, EPOLL_CTL_MOD, fd, &ev);
						}
					}
				}
				else {
					std::cerr << "unexpected ep event" << std::endl;
					//exit
				}
			}
		}
	}