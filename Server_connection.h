#pragma once
#pragma warning(disable:4996) 

#include "File_operator.h"
#include <algorithm> // for transform command

// Separator for commands
#define SEPARATOR " "

class Server_connection {
public:
	Server_connection(int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset = 1);
	std::string commandParser(std::string command);
	std::vector<std::string> extractParameters(std::string command);
	virtual ~Server_connection();
	void respondToQuery();
	int getFD();
	bool getCloseRequestStatus();
	unsigned int getConnectionId();

	SOCKET dataSocket = INVALID_SOCKET;
	SOCKET getSocket(int p, bool toBind, std::string address);
	SOCKET getSocket(char* s, bool toBind, std::string address);
private:

	SOCKET commandSocket; // command sicket aka Filedescriptor per each threaded object 
	bool closureRequested;
	std::vector<std::string> directories;
	std::vector<std::string> files;
	unsigned int connectionId;
	std::string defaultDir;

	std::string hostAddress;
	File_operator* fo; // For browsing, writing and reading 
	void sendToClient(std::string response);
	bool commandEquals(std::string a, std::string b);
	unsigned short commandOffset;
	unsigned long receivedPart;
};
