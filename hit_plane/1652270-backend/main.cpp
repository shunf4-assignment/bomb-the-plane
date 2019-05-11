#include "head.h"

int main(int argc, char *argv[]) {
	//deamonize;
	/*if (argc != 2) {
		std::cout << "Usage: ./server port" << std::endl;
		return -1;
	}
	uint32_t port = std::stoi(argv[1]);*/
	auto &mysql = Mysql::getInstance();
	mysql.connect("localhost", "u1652270", "u1652270", "db1652270");
	Application app(20270);//port);
	app.run();
	return 0;
}