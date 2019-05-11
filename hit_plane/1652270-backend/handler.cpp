#include "head.h"

std::unique_ptr<Handler> Handler::factory(int epfd, int fd, Msg &msg, std::unordered_map<int, Connection> &connections, std::unordered_map<std::string, int> &users) {
	switch (msg.type) {
	case MsgType::Login:
		return std::unique_ptr<Handler>(new LoginHandler(epfd, fd, msg, connections, users));
	case MsgType::Invite:
		return std::unique_ptr<Handler>(new InviteHandler(epfd, fd, msg, connections, users));
	case MsgType::Accept:
	case MsgType::Refuse:
		return std::unique_ptr<Handler>(new AcceptHandler(epfd, fd, msg, connections, users));
	case MsgType::Deploy:
		return std::unique_ptr<Handler>(new DeployHandler(epfd, fd, msg, connections, users));
	case MsgType::Bomb:
		return std::unique_ptr<Handler>(new BombHandler(epfd, fd, msg, connections, users));
	case MsgType::Guess:
		return std::unique_ptr<Handler>(new GuessHandler(epfd, fd, msg, connections, users));
	case MsgType::End:
		return std::unique_ptr<Handler>(new LeaveHandler(epfd, fd, msg, connections, users));
	case MsgType::Chat:
	case MsgType::Peek:
		return std::unique_ptr<Handler>(new ForwardHandler(epfd, fd, msg, connections, users));
	case MsgType::Status:
		return std::unique_ptr<Handler>(new StatusHandler(epfd, fd, msg, connections, users));
	}
	std::cerr << "factory error, unexpected msg type" << std::endl;
	std::exit(EXIT_FAILURE);
}

std::string LoginHandler::getFriends(){
	std::string ret;
	for(auto it = users.begin(); it != users.end(); ++it){
		if (it->second != fd) {
			ret += it->first;
			ret.push_back(0);
			ret += convert2String(htonl(static_cast<uint32_t>(connections.at(it->second).state)));
		}
	}
	return ret;
}

void LoginHandler::process() {
	if (connections.at(fd).state != State::Logout) {
		std::cerr << "state error while handling login" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto pos = msg.content.find('\0');
	std::string name = msg.content.substr(0, pos);
	pos = msg.content.find('\0', 32);
	std::string passwd = msg.content.substr(32, pos - 32);
	auto & mysql = Mysql::getInstance();
	auto res = mysql.query("select passwd from user where name=\'" + name + "\'");
	if (res.empty()) {
		std::clog << "login fail due to user name error" << std::endl;
		Msg reply{ MsgType::RLoginUsernameError, "" };
		connections.at(fd).addMsg(reply);
	}
	else if (res[0][0] == passwd) {
		std::clog << "login succeed, user: " << name << std::endl;
		auto it = users.find(name);
		if (it != users.end()) {
			std::clog << "old login disconnected" << std::endl;
			Msg informOffline{MsgType::Reset, ""};

			Connection &oldLogin = connections.at(it->second);

			oldLogin.addMsg(informOffline);
			while (!oldLogin.obuf.empty())
				connections.at(it->second).output();
			handleDisconnection(epfd, it->second, connections, users);
		}
		users[name] = fd;
		auto res = mysql.query("select state, opponent, h1, t1, h2, t2, h3, t3, oh1, ot1, oh2, ot2, oh3, ot3, board, op_board from game_record where player=\'" + name + "\'");
		if (res.empty()){
			std::string s = convert2String(htonl(static_cast<uint32_t>(State::Idle)));
			s += getFriends();
			Msg reply{ MsgType::RLoginOk, s };
			auto &conn = connections.at(fd);
			conn.addMsg(reply);
			conn.name = name;
			conn.state = State::Idle;
		}
		else {
			auto &conn = connections.at(fd);
			conn.name = name;
			uint32_t state = std::stoi(res[0][0]);
			conn.state = static_cast<State>(state);
			std::string s = convert2String(htonl(state));
			conn.opname = res[0][1];
			s += res[0][1];
			s.push_back(0);
			char ht[12];
			for (int i = 0; i < 12; ++i) {
				ht[i] = std::stoi(res[0][i + 2]);
				char mask = 0x1 << 7;
				char mask2 = 0xFF >> 1;
				if (i < 6){
					if (ht[i] & mask)
						s.push_back(ht[i] & mask2);
					else
						s.push_back(100);
				}
				else
					s.push_back(ht[i]);
			}
			conn.board.recovery(ht[0], ht[1], ht[2], ht[3], ht[4], ht[5], res[0][14]);
			s += conn.board.filter();
			s += res[0][15];
			s += getFriends();
			Msg reply{MsgType::RLoginOk, s};
			conn.addMsg(reply);
		}
	}
	else {
		std::clog << "login fail due to password error" << std::endl;
		Msg reply{ MsgType::RLoginPasswordError, "" };
		connections.at(fd).addMsg(reply);
	}
	addEpOutEv(epfd, fd);
}

void InviteHandler::process() {
	if (connections.at(fd).state != State::Idle) {
		std::cerr << "state error while handling invite" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	std::clog << connections.at(fd).name << " invite " << msg.content << std::endl;
	auto it = users.find(msg.content);
	if (it == users.end()) {
		Msg rep{ MsgType::RInviteErrorLogout,"" };
		std::clog << "invite fail due to user name error" << std::endl;
		connections.at(fd).addMsg(rep);
	}
	else if (connections.at(it->second).state != State::Idle) {
		Msg rep{MsgType::RInviteErrorBusy, ""};
		std::clog << "invite fail due to user is busy" << std::endl;
		connections.at(fd).addMsg(rep);
	}
	else {
		Msg rep{ MsgType::RInviteOk,"" };
		std::clog << "invite succeed" << std::endl;
		connections.at(fd).addMsg(rep);
		int op = it->second;
		connections.at(fd).opname = it->first;
		connections.at(fd).state = State::P1WaitAcc;
		Msg invite{ MsgType::Invite, connections.at(fd).name };
		connections.at(op).addMsg(invite);
		connections.at(op).opname = connections.at(fd).name;
		connections.at(op).state = State::P2WaitAcc;
		addEpOutEv(epfd, op);
	}
	addEpOutEv(epfd, fd);
}

void AcceptHandler::process() {
	if (connections.at(fd).state != State::P2WaitAcc) {
		std::cerr << "state error while handling accept" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	if (msg.type == MsgType::Accept){
		Msg reply{MsgType::RAccept, ""};
		std::clog << conn.name << " accept invitaion" << std::endl;
		conn.addMsg(reply);
		Msg inform{MsgType::Accept, ""};
		opconn.addMsg(inform);
		conn.state = State::Deploy;
		opconn.state = State::Deploy;
	}
	else {
		Msg reply{MsgType::RRefuse, ""};
		std::clog << conn.name << " refuse invitaion" << std::endl;
		conn.addMsg(reply);
		Msg inform{MsgType::Refuse, ""};
		opconn.addMsg(inform);
		conn.state = State::Idle;
		opconn.state = State::Idle;
	}
	addEpOutEv(epfd, opconn.sockfd);
	addEpOutEv(epfd, fd);
}

void DeployHandler::process() {
	if (connections.at(fd).state != State::Deploy && connections.at(fd).state != State::OpDeployed) {
		std::cerr << "state error while handling deploy" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	char ht[6];
	for (int i = 0; i < 6; ++i)
		ht[i] = msg.content[i];
	addEpOutEv(epfd, fd);
	if (opconn.board.reset(ht[0], ht[1], ht[2], ht[3], ht[4], ht[5]) == -1) {
		Msg reply{MsgType::RDeployError, ""};
		std::clog << conn.name << "\'s deploy error" << std::endl;
		conn.addMsg(reply);
		return;
	}
	Msg reply{MsgType::RDeployOk, ""};
	std::clog << conn.name << "\'s deploy ok" << std::endl;
	conn.addMsg(reply);
	Msg inform{MsgType::Deploy, ""};
	opconn.addMsg(inform);
	addEpOutEv(epfd, opconn.sockfd);
	if (opconn.state == State::Deploy){
		conn.state = State::WaitDeploy;
		opconn.state = State::OpDeployed;
	}
	else if (opconn.state == State::WaitDeploy) {
		opconn.state = State::MyTurn;
		conn.state = State::WaitOp;
	} 
	else {
		std::cerr << "opponent state error while handling deploy" << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

void BombHandler::process() {
	if (connections.at(fd).state != State::MyTurn) {
		std::cerr << "state error while handling bomb" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	char pos = msg.content[0];
	std::string s;
	std::clog << conn.name << ' ' << std::endl;
	auto g = conn.board.bomb(pos);
	s.push_back(static_cast<char>(g));
	Msg rep{MsgType::RBomb, s};
	conn.addMsg(rep);
	opconn.addMsg(msg);
	conn.state = State::WaitOp;
	opconn.state = State::MyTurn;
	addEpOutEv(epfd, fd);
	addEpOutEv(epfd, opconn.sockfd);
}

void GuessHandler::process() {
	if (connections.at(fd).state != State::MyTurn) {
		std::cerr << "state error while handling guess" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	auto head = msg.content[0];
	auto tail = msg.content[1];
	std::clog << conn.name << ' ' << std::endl;
	auto res = conn.board.guess(head, tail);
	addEpOutEv(epfd, fd);
	addEpOutEv(epfd, opconn.sockfd);
	if (res && conn.board.down.size() == 3){
		Msg rep{MsgType::Win, ""};
		conn.addMsg(rep);
		Msg inform{MsgType::Lose, ""};
		opconn.addMsg(inform);
		conn.state = State::Idle;
		opconn.state = State::Idle;
	}
	else {
		std::string s;
		if (res)
			s = std::string{1};
		else
			s = std::string{0};
		Msg rep{MsgType::RGuess, s};
		conn.addMsg(rep);
		opconn.addMsg(msg);
		conn.state = State::WaitOp;
		opconn.state = State::MyTurn;
	}
}

void LeaveHandler::process() {
	if (connections.at(fd).state == State::Idle || connections.at(fd).state == State::Logout) {
		std::cerr << "state error while handling leave" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	std::clog << conn.name << " has left." << std::endl;
	Msg rep{MsgType::REnd, ""};
	conn.addMsg(rep);
	opconn.addMsg(msg);
	addEpOutEv(epfd, fd);
	addEpOutEv(epfd, opconn.sockfd);
	conn.state = State::Idle;
	opconn.state = State::Idle;
}

void ForwardHandler::process() {
	if (connections.at(fd).state != State::MyTurn || connections.at(fd).state != State::Deploy || connections.at(fd).state != State::OpDeployed) {
		std::cerr << "state error while handling forward" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	auto &conn = connections.at(fd);
	auto &opconn = connections.at(users.at(conn.opname));
	
	opconn.addMsg(msg);
	addEpOutEv(epfd, opconn.sockfd);
}

void StatusHandler::process() {
	if (connections.at(fd).state == State::Logout) {
		std::cerr << "state error while handling status msg" << std::endl;
		std::exit(EXIT_FAILURE);
	}

	auto &conn = connections.at(fd);
	std::string s = convert2String(htonl(static_cast<uint32_t>(conn.state)));

	if (conn.state != State::Idle) {
		auto &opconn = connections.at(users.at(conn.opname));
		s += conn.opname;
		s.push_back(0);
		char ht[12];
		ht[0] = conn.board.head[0];
		ht[1] = conn.board.tail[0];
		ht[2] = conn.board.head[1];
		ht[3] = conn.board.tail[1];
		ht[4] = conn.board.head[2];
		ht[5] = conn.board.tail[2];

		ht[6] = opconn.board.head[0];
		ht[7] = opconn.board.tail[0];
		ht[8] = opconn.board.head[1];
		ht[9] = opconn.board.tail[1];
		ht[10] = opconn.board.head[2];
		ht[11] = opconn.board.tail[2];

		for (int i = 0; i < 12; ++i) {
			char mask = 0x1 << 7;
			char mask2 = 0xFF >> 1;
			if (i < 6){
				if (ht[i] & mask)
					s.push_back(ht[i] & mask2);
				else
					s.push_back(100);
			}
			else
				s.push_back(ht[i]);
		}
		
		s += conn.board.filter();
		s += static_cast<std::string>(opconn.board);
	}
	s += getFriends();
	Msg reply{MsgType::RStatus, s};
	conn.addMsg(reply);

	addEpOutEv(epfd, fd);
}

std::string StatusHandler::getFriends(){
	std::string ret;
	for(auto it = users.begin(); it != users.end(); ++it){
		if (it->second != fd) {
			ret += it->first;
			ret.push_back(0);
			ret += convert2String(htonl(static_cast<uint32_t>(connections.at(it->second).state)));
		}
	}
	return ret;
}