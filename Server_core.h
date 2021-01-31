#pragma once

#include "Server_connection.h"

class Server_core
{
public:
	Server_core(unsigned int port, std::string dir);
	~Server_core();
private:
	std::string default_root_dir;
	bool shutdown;
	unsigned int currentConnectionId;
	SOCKET current_socket;
	fd_set sockets; // set of socket file descriptors we want to wake up for
	const short commandOffset = 1;

	unsigned int maxConnectionsInQuery; // number of connections in query
	int sflags; // Socket fd flags
	std::vector<Server_connection*> connections;// Manage the connected sockets / connections in a list with an iterator
	int highSock; // Highest #'d file descriptor
	sockaddr_in addr;
	struct sockaddr_storage addrStorage;
	socklen_t addrLength;
	sockaddr_in cli;
	socklen_t cli_size;

	int start();
	void initSockets(int port);
	void buildSelectList();
	void readSockets();
	int handleNewConnection();
	void freeAllConnections();
	void setNonBlocking(SOCKET sock);
};


