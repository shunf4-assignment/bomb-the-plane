#include "head.h"

void addEpOutEv(int epfd, int fd) {
	epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT;
	ev.data.fd = fd;
	call(epoll_ctl, "epoll_ctl mod", epfd, EPOLL_CTL_MOD, fd, &ev);
}

void handleDisconnection(int epfd, int fd, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) {
	auto &conn = connections.at(fd);
	auto it = users.find(conn.opname);
	if (conn.state != State::MyTurn && conn.state != State::WaitOp || true) {
		if (it != users.end()){
			auto &opconn = connections.at(it->second);
			if (opconn.state != State::Idle){
				Msg msg{MsgType::End, ""};
				opconn.addMsg(msg);
				addEpOutEv(epfd, opconn.sockfd);
				opconn.state = State::Idle;
			}
		}
	}
	else if (it != users.end()){
		auto &mysql = Mysql::getInstance();
		auto &opconn = connections.at(it->second);
		std::string s = "insert into game_record values(\'";
		s += conn.name + "\', \'";
		s += opconn.name + "\', \'";
		for (int i = 0; i < 3; i++){
			s += std::to_string(static_cast<unsigned char>(conn.board.head[i])) + "\', \'";
			s += std::to_string(static_cast<unsigned char>(conn.board.tail[i])) + "\', \'";
		}
		for (int i = 0; i < 3; i++){
			s += std::to_string(static_cast<unsigned char>(opconn.board.head[i])) + "\', \'";
			s += std::to_string(static_cast<unsigned char>(opconn.board.tail[i])) + "\', \'";
		}
		s += std::to_string(static_cast<int>(conn.state)) + "\', \'";
		s += static_cast<std::string>(conn.board) + "\', \'";
		s += static_cast<std::string>(opconn.board) + "\')";
		mysql.realQuery(s, s.length());
	}
	else {
		auto &mysql = Mysql::getInstance();
		std::string s = "insert into game_record values(\'";
		s += conn.name + "\', \'";
		s += conn.opname + "\', \'";
		for (int i = 0; i < 3; i++){
			s += std::to_string(static_cast<unsigned char>(conn.board.head[i])) + "\', \'";
			s += std::to_string(static_cast<unsigned char>(conn.board.tail[i])) + "\', \'";
		}
		auto res = mysql.query("select h1, t1, h2, t2, h3, t3, board from game_record where name =\'" + conn.opname + "\')");
		for (int i = 0; i < 6; i++)
			s += res[0][i] + "\', \'";
		s += std::to_string(static_cast<int>(conn.state)) + "\', \'";
		s += static_cast<std::string>(conn.board) + "\', \'";
		s += res[0][6] + "\')";
		mysql.realQuery(s, s.length());
	}
	users.erase(conn.name);
	connections.erase(fd);
	call(epoll_ctl, "epoll_ctl del", epfd, EPOLL_CTL_DEL, fd, nullptr);
}
