#include <iostream>

#include "Server_core.h"



int main(int argc, char** argv) {
	unsigned int port = 2123; // Port to listen on (>1024 for no root permissions required)
	//pls use \\ instead /
	std::string default_root_dir = "D:\\test\\"; 

	//WSA initialise block
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0) {
		// Tell the user that we could not find a usable Winsock DLL.
		printf("WSAStartup failed with error\n"); 
		return (EXIT_FAILURE);
	}

	Server_core* myServer = new Server_core(port, default_root_dir);



	delete myServer; // Close connection
	WSACleanup();

	return (EXIT_SUCCESS);
}