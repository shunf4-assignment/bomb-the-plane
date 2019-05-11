#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <numeric>
#include <limits>
#include <random>
#include <memory>
#include <set>

#include <cstring>
#include <cmath>
#include <cstdio>
#include <mysql/mysql.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "enum.h"

constexpr auto MAX_EVENTS = 1024;
constexpr auto MAX_BUFFER_LENGTH = 1024;
constexpr auto BACKLOG = 20;

template<typename F, typename ...Args>
int call(const F &f, const char *errMsg, Args... args) {
	int ret = f(args...);
	if (ret == -1) {
		perror(errMsg);
		std::exit(EXIT_FAILURE);
	}
	return ret;
}

template<typename T>
std::string convert2String(T t) {
	return std::string(reinterpret_cast<const char *>(&t), sizeof(t));
}

void addEpOutEv(int epfd, int fd);

class Mysql {
public:
	static Mysql &getInstance() {
		static Mysql instance;
		return instance;
	}
	Mysql(const Mysql &mysql) = delete;
	Mysql &operator=(const Mysql &mysql) = delete;
	void connect(std::string host, std::string user, std::string passwd, std::string db, unsigned int port = 0, const char *unix_socket = nullptr, unsigned long clientflag = 0);
	using Res = std::vector<std::vector<std::string>>;
	Res query(std::string q);
	void realQuery(std::string q, int len);
private:
	MYSQL *mysql;
	Mysql() :mysql(mysql_init(nullptr)) {
		if (mysql == nullptr) {
			std::cerr << "mysql_init failure" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
};

struct Board {
	std::array<Grid, 100> board;
	std::set<char> down;
	char head[3];
	char tail[3];
	Board() {
		std::fill(board.begin(), board.end(), Grid::Blank);
	}
	int reset(char head1, char tail1, char head2, char tail2, char head3, char tail3);
	int draw(char head, char tail);
	void recovery(char head1, char tail1, char head2, char tail2, char head3, char tail3, std::string s);
	std::string filter();
	Grid bomb(char pos);
	bool guess(char head, char tail);
	explicit operator std::string();
};

struct Msg {
	MsgType type;
	std::string content;
	explicit operator std::string() {
		u_int32_t len = htonl(sizeof(type) + content.length());
		u_int32_t t = htonl(static_cast<u_int32_t>(type));
		return convert2String(len) + convert2String(t) + content;
	}
};


struct ReadBuffer {
	std::array<char, MAX_BUFFER_LENGTH> buf;
	decltype(buf.size()) iptr;
	decltype(buf.size()) optr;
	ReadBuffer() :iptr(0), optr(0) {}
};

struct Connection {
	int sockfd;
	std::vector<char> obuf;
	Board board;
	ReadBuffer ibuf;
	State state;
	std::string name;
	std::string opname;

	Connection() = delete;
	Connection(int fd) :sockfd(fd), state(State::Logout) {}
	int output();
	int input();
	Msg getMsg();
	void addMsg(Msg &msg);
};

class Handler {
protected:
	int epfd;
	int fd;
	Msg &msg;
	std::unordered_map<int, Connection> &connections;
	std::unordered_map<std::string, int> &users;
public:
	virtual void process() = 0;
	static std::unique_ptr<Handler> factory(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users);
	Handler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :epfd(epfd), fd(fd), msg(msg), connections(connections), users(users) {}
};

class LoginHandler final :public Handler {
public:
	LoginHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
	std::string getFriends();
};

class InviteHandler final :public Handler {
public:
	InviteHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class AcceptHandler final :public Handler {
public:
	AcceptHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class DeployHandler final :public Handler {
public:
	DeployHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class BombHandler final :public Handler {
public:
	BombHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class GuessHandler final :public Handler {
public:
	GuessHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class LeaveHandler final :public Handler {
public:
	LeaveHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class ForwardHandler final :public Handler {
public:
	ForwardHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
};

class StatusHandler final :public Handler {
public:
	StatusHandler(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) :Handler(epfd, fd, msg, connections, users) {}
	void process() override;
	std::string getFriends();
};

class Application {
public:
	Application() = delete;
	Application(uint32_t port);
	void run();
private:
	int listenfd;
	int epollfd;
	epoll_event events[MAX_EVENTS];
	std::unordered_map<int, Connection> connections;
	std::unordered_map<std::string, int> users;
	void acceptConn();
};

void handleDisconnection(int epfd, int fd, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users);
