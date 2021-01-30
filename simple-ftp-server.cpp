#pragma comment(lib, "Ws2_32.lib")

#include "Server_core.h"
#include <cstdlib>
#include <iostream>




int main(int argc, char** argv) {
	unsigned short commandOffset = 1;
	unsigned int port = 2123; // Port to listen on (>1024 for no root permissions required)
	std::string dir = "D:/test/"; // Default dir

	//WSA initialise block
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		printf("WSAStartup failed with error\n");
		return (EXIT_FAILURE);
	}

	servercore* myServer = new servercore(port, dir, commandOffset);
	delete myServer; // Close connection

	WSACleanup();

	return (EXIT_SUCCESS);
}