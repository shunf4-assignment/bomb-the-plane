#include "head.h"

void Mysql::connect(std::string host, std::string user, std::string passwd, std::string db, unsigned int port, const char *unix_socket, unsigned long clientflag) {
	if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, unix_socket, clientflag) == nullptr) {
		std::cerr << "mysql_real_connect failure" << std::endl;
		std::exit(EXIT_FAILURE);
	}
	//mysql_set_character_set(mysql, "gbk");
}

Mysql::Res Mysql::query(std::string q) {
	if (mysql_query(mysql, q.c_str())) {
		std::cerr << "mysql_query failure" << std::endl;
		return {};
	}
	std::clog << "mysql query ok. query = \"" << q << "\"" << std::endl;
	auto res = mysql_store_result(mysql);
	if (res == nullptr) {
		std::cerr << "mysql_store_result failure" << std::endl;
		return {};
	}
	Res ret(mysql_num_rows(res), std::vector<std::string>(mysql_num_fields(res)));
	for (auto &row : ret) {
		auto res_row = mysql_fetch_row(res);
		auto length = mysql_fetch_lengths(res);
		for (auto i = 0; i < row.size(); i++)
			row[i] = std::string(res_row[i], length[i]);
	}
	mysql_free_result(res);
	return ret;
}

void Mysql::realQuery(std::string q, int len) {
	if (mysql_real_query(mysql, q.data(), len)) 
		std::cerr << "mysql_real_query failure" << std::endl;
	else
		std::clog << "mysql real query ok. query = \"" << q << "\", length: " << len <<std::endl;
}