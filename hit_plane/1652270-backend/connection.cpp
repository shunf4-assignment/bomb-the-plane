#include "head.h"

int Connection::output() {
	int res = write(sockfd, obuf.data(), obuf.size());
	if (res < 0) {
		if (errno != EWOULDBLOCK) {
			perror("write");
			std::exit(EXIT_FAILURE);
		}
	}
	std::string s(obuf.begin(), obuf.begin() + res);
	std::clog << "write " << res << " bytes: \"" << s << '\"' << std::endl;
	if (res == obuf.size())
		obuf.clear();
	else
		obuf.erase(obuf.begin(), obuf.begin() + res);
	return res;
}

int Connection::input() {
	int res = read(sockfd, ibuf.buf.data(), MAX_BUFFER_LENGTH - ibuf.iptr);
	if (res < 0) {
		if (errno == EWOULDBLOCK) {
			return -1;
		}
		else if (errno == ECONNRESET) {
			return 0;
		} else {
			perror("read");
			std::exit(EXIT_FAILURE);
		}
	}
	else if (res == 0) {
		return 0;
	}
	else {
		ibuf.iptr += res;
		return res;
	}
}

Msg Connection::getMsg() {
	if (ibuf.iptr - ibuf.optr < 4)
		return { MsgType::Null,"" };
	u_int32_t len = ntohl(*(reinterpret_cast<u_int32_t *>(ibuf.buf.data() + ibuf.optr)));
	if (ibuf.iptr - ibuf.optr - sizeof(len) < len)
		return { MsgType::Null,"" };
	Msg ret;
	ret.type = static_cast<MsgType>(ntohl(*(reinterpret_cast<u_int32_t *>(ibuf.buf.data() + ibuf.optr + sizeof(len)))));
	ret.content.assign(ibuf.buf.data() + ibuf.optr + sizeof(len) + sizeof(MsgType), len - sizeof(MsgType));
	ibuf.optr += sizeof(len) + len;
	if (ibuf.optr == ibuf.iptr) {
		ibuf.optr = 0;
		ibuf.iptr = 0;
	}
	std::clog << "get Msg, type:" << static_cast<int>(ret.type) << ", content: \"" << ret.content << '\"' << std::endl;
	return ret;
}

void Connection::addMsg(Msg &msg) {
	std::string s(static_cast<std::string>(msg));
	obuf.insert(obuf.end(), s.begin(), s.end());
}