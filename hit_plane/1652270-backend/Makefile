all : _btp_server

_btp_server : application.cpp board.cpp connection.cpp handler.cpp main.cpp misc.cpp mysql.cpp enum.h head.h
	g++ -o_btp_server --std=c++11 $(shell mysql_config --cflags) application.cpp board.cpp connection.cpp handler.cpp main.cpp misc.cpp mysql.cpp $(shell mysql_config --libs)

.PHONY : clean

clean :
	$(RM) _btp_server