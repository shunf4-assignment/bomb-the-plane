#include "head.h"

int Board::reset(char head1, char tail1, char head2, char tail2, char head3, char tail3) {
	std::fill(board.begin(), board.end(), Grid::Blank);
	if (draw(head1, tail1) == -1 || draw(head2, tail2) == -1 || draw(head3, tail3) == -1) {
		std::fill(board.begin(), board.end(), Grid::Blank);
		return -1;
	}
	head[0] = head1;
	head[1] = head2;
	head[2] = head3;
	tail[0] = tail1;
	tail[1] = tail2;
	tail[2] = tail3;
	down.clear();
	return 0;
}

void Board::recovery(char head1, char tail1, char head2, char tail2, char head3, char tail3, std::string s) {
	head[0] = head1;
	head[1] = head2;
	head[2] = head3;
	tail[0] = tail1;
	tail[1] = tail2;
	tail[2] = tail3;
	down.clear();
	char mask = 0x1 << 7;
	if (head1 & mask)
		down.insert(head1);
	if (head2 & mask)
		down.insert(head2);
	if (head3 & mask)
		down.insert(head3);
	for (int i = 0; i < 100; ++i)
		board[i] = static_cast<Grid>(s[i]);
}

std::string Board::filter() {
	std::string ret;
	for(int i = 0; i < 100; ++i) {
		if (isHit(board[i]))
			ret.push_back(static_cast<char>(board[i]));
		else
			ret.push_back(static_cast<char>(Grid::Blank));
	}
	return ret;
}

int Board::draw(char head, char tail) {
	if (head < 0 || head > 99 || tail < 0 || tail > 99)
		return -1;
	if (board[head] != Grid::Blank || board[tail] != Grid::Blank)
		return -1;
	board[head] = Grid::Head;
	board[tail] = Grid::Tail;
	if (head / 10 == tail / 10 && std::abs(head - tail) == 3 && head / 10 >= 2 && head / 10 <= 7) {
		int step = (tail - head) / 3;
		if (board[head + step] != Grid::Blank || board[head + 2 * step] != Grid::Blank)
			return -1;
		for (int i = head + step - 20; i <= head + step + 20; i += 10) {
			if (board[i] != Grid::Blank)
				return -1;
			else
				board[i] = Grid::Body;
		}
		board[head + 2 * step] = Grid::Body;
		if (board[tail - 10] != Grid::Blank || board[tail + 10] != Grid::Blank)
			return -1;
		board[tail - 10] = Grid::Body;
		board[tail + 10] = Grid::Body;
	}
	else if (head % 10 == tail % 10 && std::abs(head - tail) == 30 && head % 10 >= 2 && head % 10 <= 7) {
		int step = (tail - head) / 3;
		if (board[head + step] != Grid::Blank || board[head + 2 * step] != Grid::Blank)
			return -1;
		for (int i = head + step - 2; i <= head + step + 2; i++) {
			if (board[i] != Grid::Blank)
				return -1;
			else
				board[i] = Grid::Body;
		}
		board[head + 2 * step] = Grid::Body;
		if (board[tail - 1] != Grid::Blank || board[tail + 1] != Grid::Blank)
			return -1;
		board[tail - 1] = Grid::Body;
		board[tail + 1] = Grid::Body;
	}
	else {
		return -1;
	}
	return 0;
}




Grid Board::bomb(char pos) {
	std::clog << "bomb " << static_cast<int>(pos) << " it's ";
	auto &g = board[pos];
	if (!isHit(g)) {
		g = g | Grid::Hit;
	}
	if (isHead(g)){
		std::clog << "head" << std::endl;
		return g;
	}
	if (isTail(g)){
		std::clog << "body" << std::endl;
		return g;
	}
	if (isBody(g)){
		std::clog << "body" << std::endl;
		return g;
	}
	std::clog << "blank" << std::endl;
	return g;
}

bool Board::guess(char h, char t) {
	std::clog << "guess head: " << static_cast<int>(h) << " tail: " << static_cast<int>(t);
	char mask = 0x1 << 7;
	char m = 0xFF >> 1;
	if (h == (head[0]&m) && t == (tail[0]&m)){
		head[0] |= mask;
		tail[0] |= mask;
		down.insert(head[0]);
		std::clog << ". It's a plane" << std::endl;
		return true;
	}
	else if (h == (head[1]&m) && t == (tail[1]&m)) {
		head[1] |= mask;
		tail[1] |= mask;
		down.insert(head[1]);
		std::clog << ". It's a plane" << std::endl;
		return true;
	}
	else if (h == (head[2]&m) && t == (tail[2]&m)) {
		head[2] |= mask;
		tail[2] |= mask;
		down.insert(head[2]);
		std::clog << ". It's a plane" << std::endl;
		return true;
	}
	std::clog << ". It's not a plane" << std::endl;
	return false;
}

Board::operator std::string() {
	std::string ret;
	for (int i = 0; i < 100; ++i)
		ret.push_back(static_cast<char>(board[i]));
	return ret;
}