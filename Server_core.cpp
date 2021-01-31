#include "Server_core.h"

Server_core::Server_core(unsigned int port, std::string dir) : default_root_dir(dir), shutdown(false), currentConnectionId(0) {
	std::wstring stemp = std::wstring(default_root_dir.begin(), default_root_dir.end());
	LPCWSTR sw = stemp.c_str();
	if (!SetCurrentDirectory(sw)) {
		std::cerr << "Directory could not be changed to '" << dir << "'!" << std::endl;
	}
	initSockets(port);
	start();

}

// Free up used memory by cleaning up all the object variables;
Server_core::~Server_core() {
	std::cout << "Server shutdown" << std::endl;
	closesocket(this->current_socket);
	this->freeAllConnections(); // Deletes all connection objects and frees their memory
}

// Builds the list of sockets to keep track on and removes the closed ones
void Server_core::buildSelectList() {
	FD_ZERO(&(sockets));

	FD_SET(current_socket, &(sockets));

	// Iterates over all the possible connections and adds those sockets to the fd_set and erases closed ones
	std::vector<Server_connection*>::iterator iter = this->connections.begin();
	while (iter != this->connections.end()) {
		if ((*iter)->getCloseRequestStatus() == true) { // This connection was closed, flag is set -> remove its corresponding object and free the memory
			std::cout << "Connection with Id " << (*iter)->getConnectionId() << " closed! " << std::endl;
			delete (*iter); // Clean up
			this->connections.erase(iter); // Delete it from our vector
			if (this->connections.empty() || (iter == this->connections.end()))
				return; // Don't increment the iterator when there is nothing to iterate over - avoids crash
		}
		else {
			int currentFD = (*iter)->getFD();
			if (currentFD != 0) {
				FD_SET(currentFD, &(this->sockets)); // Adds the socket file descriptor to the monitoring for select
				if (currentFD > this->highSock)
					this->highSock = currentFD; // We need the highest socket for select
			}
		}
		++iter;
	}
}

// Clean up everything
void Server_core::freeAllConnections() {
	std::vector<Server_connection*>::iterator iter = this->connections.begin();
	while (iter != this->connections.end()) {
		delete (*(iter++)); // Clean up, issue destructor implicitly
	}
	this->connections.clear(); // Delete all deleted connections also from our vector
}

// Accepts new connections and stores the connection object with fd in a vector
int Server_core::handleNewConnection() {
	SOCKET commandSocket; // Socket file descriptor for incoming connections

	this->cli_size = sizeof(this->cli);
	commandSocket = accept(this->current_socket, (struct sockaddr*) & cli, &cli_size);
	if (commandSocket < 0) {
		std::cerr << "Something went wrong, new connection could not be handled" << std::endl;
		try {
			closesocket(commandSocket);
		}
		catch (std::exception e) {
			std::cerr << e.what() << std::endl;
		}
		return (EXIT_FAILURE);
	}

	std::string to_send = "220 Welcome to  server.\r\n";
	send(commandSocket, to_send.c_str(), to_send.size(), 0);

	// Gets the socket fd flags and add the non-blocking flag to the sfd
	this->setNonBlocking(commandSocket);


	// Get the client IP address
	char ipstr[INET6_ADDRSTRLEN];
	int port;
	this->addrLength = sizeof this->addrStorage;
	getpeername(commandSocket, (struct sockaddr*) & this->addrStorage, &(this->addrLength));
	std::string hostId = "";
	if (this->addr.sin_family == AF_INET) {
		struct sockaddr_in* commandSocket = (struct sockaddr_in*) & (this->addrStorage);
		port = ntohs(commandSocket->sin_port);
		inet_ntop(AF_INET, &commandSocket->sin_addr, ipstr, sizeof ipstr);
		hostId = (std::string)ipstr;
	}

	printf("Connection accepted: FD=%d - Slot=%lu - Id=%d \n", commandSocket, (this->connections.size() + 1), ++(this->currentConnectionId));

	// The new connection (object)
	Server_connection* conn = new Server_connection(commandSocket, currentConnectionId, default_root_dir, hostId, commandOffset); // The connection vector

	// Add it to our list for better management / convenience
	this->connections.push_back(conn);
	return (EXIT_SUCCESS);
}

// Data ready to read at a socket, either accept a new connection or handle the incoming data over an already opened socket
void Server_core::readSockets() {
	//First check our "listening" socket, and then check the sockets in connectlist
	// If a client is trying to connect() to our listening socket, select() will consider that as the socket being 'readable' and thus, if the listening socket is part of the fd_set, accept a new connection
	if (FD_ISSET(this->current_socket, &(this->sockets))) {
		if (this->handleNewConnection()) {  
			std::cout << "smth went wrong on " << __FUNCTION__ << std::endl;
			return;
		}
	}
	// Check connectlist for available data
	// Run through our sockets and check to see if anything happened with them, if so 'service' them
	for (unsigned int listnum = 0; listnum < this->connections.size(); listnum++) {
		if (FD_ISSET(this->connections.at(listnum)->getFD(), &(this->sockets))) {
			this->connections.at(listnum)->respondToQuery(); // Now that data is available
		}
	}
}

// Server entry point and main loop accepting and handling connections
int Server_core::start() {
	struct timeval timeout; // Timeout for select
	int readsocks; // Number of sockets ready for reading
	timeout.tv_sec = 1; // Timeout = 1 sec
	timeout.tv_usec = 0;
	// Wait for connections, main server loop
	while (!this->shutdown) {

		this->buildSelectList(); // Clear out data handled in the previous iteration, clear closed sockets

		// Multiplexes between the existing connections regarding to data waiting to be processed on that connection
		readsocks = select(this->highSock + 1, &(this->sockets), (fd_set*)0, (fd_set*)0, &timeout);

		if (readsocks < 0) {
			std::cerr << "Error calling select" << std::endl;
			return (EXIT_FAILURE);
		}

		this->readSockets(); // Handle the sockets (accept new connections or handle incoming data or do nothing [if no data])
	}
	return (EXIT_SUCCESS);
}

// Sets the given socket to non-blocking mode
void Server_core::setNonBlocking(SOCKET sock) {

	BOOL l = TRUE;
	if (SOCKET_ERROR == ioctlsocket(sock, FIONBIO, (unsigned long*)&l))
	{
		std::cerr << "Error setting socket to non-blocking" << std::endl;
		int res = WSAGetLastError();
	}
	return;
}

// Initialization of sockets / socket list with options and error checking
void Server_core::initSockets(int port) {
	int reuseAllowed = 0;
	this->maxConnectionsInQuery = 50;
	this->addr.sin_family = AF_INET;
	this->addr.sin_port = htons(port);
	this->addr.sin_addr.s_addr = INADDR_ANY; // Server can be connected to from any host
	current_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->current_socket == -1) {
		std::cerr << "socket() failed (1) " << std::endl;
		return;
	}
	else if (setsockopt(this->current_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)(&reuseAllowed), sizeof(reuseAllowed)) < 0) { //  enable reuse of socket, even when it is still occupied
		std::cerr << "setsockopt() failed (2)" << std::endl;
		closesocket(this->current_socket);
		return;
	}
	this->setNonBlocking(current_socket);
	if (bind(this->current_socket, (struct sockaddr*) & addr, sizeof(addr)) == -1) {
		std::cerr << ("bind() failed (3)") << std::endl;
		closesocket(this->current_socket);
		return;
	} // 2nd parameter (backlog): number of connections in query, can be also set SOMAXCONN
	else if (listen(this->current_socket, this->maxConnectionsInQuery) == -1) {
		std::cerr << ("listen() failed (4)") << std::endl;
		closesocket(this->current_socket);
		return;
	}
	this->highSock = this->current_socket; // This is the first (and the main listening) socket
	std::cout << "Server started and listening at port " << port << ", default server directory is " << this->default_root_dir << std::endl;
}
