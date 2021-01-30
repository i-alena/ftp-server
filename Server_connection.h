#pragma once
#pragma warning(disable:4996) 
#include "File_operator.h"

#include <vector>
#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <sys/types.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <algorithm> // for transform command
#include <sstream>
#include <string>
#include <random>
#include <time.h>
#include <xstring>

#include <winnt.h>


// Separator for commands
#define SEPARATOR " "

class serverconnection {
public:
	serverconnection(int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset = 1);
	std::string commandParser(std::string command);
	std::vector<std::string> extractParameters(std::string command);
	virtual ~serverconnection();
	void respondToQuery();
	int getFD();
	bool getCloseRequestStatus();
	unsigned int getConnectionId();

	SOCKET data_socket = INVALID_SOCKET;
	SOCKET get_socket(int p, bool toBind, std::string address);
	SOCKET get_socket(char* s, bool toBind, std::string address);
	std::string to_send;

	std::string msg;
private:
	int fd; // command sicket aka Filedescriptor per each threaded object 
	bool closureRequested;
	std::vector<std::string> directories;
	std::vector<std::string> files;
	unsigned int connectionId;
	std::string dir;


	std::string full_path = dir;

	std::string hostAddress;
	bool uploadCommand;
	bool downloadCommand;
	std::string parameter;
	fileoperator* fo; // For browsing, writing and reading
	void sendToClient(char* response, unsigned long length);
	void sendToClient(std::string response);
	bool commandEquals(std::string a, std::string b);
	unsigned short commandOffset;
	unsigned long receivedPart;
};