#include "Server_core.h"
#include "Server_connection.h"

servercore::servercore(unsigned int port, std::string dir, unsigned short commandOffset) : dir(dir), commandOffset(commandOffset), shutdown(false), connId(0) {
	if (_chdir(dir.c_str()))
		std::cerr << "Directory could not be changed to '" << dir << "'!" << std::endl;

	this->initSockets(port);
	this->start();
}

// Free up used memory by cleaning up all the object variables;
servercore::~servercore() {
	std::cout << "Server shutdown" << std::endl;
	closesocket(this->s);
	this->freeAllConnections(); // Deletes all connection objects and frees their memory
}

// Builds the list of sockets to keep track on and removes the closed ones
void servercore::buildSelectList() {
	// First put together fd_set for select(), which consists of the sock variable in case a new connection is coming in, plus all the already accepted sockets
	// FD_ZERO() clears out the fd_set called socks, so that it doesn't contain any file descriptors
	FD_ZERO(&(this->socks));

	// FD_SET() adds the file descriptor "sock" to the fd_set so that select() will return if a connection comes in on that socket (in which case accept() is called, etc.)
	FD_SET(this->s, &(this->socks));

	// Iterates over all the possible connections and adds those sockets to the fd_set and erases closed ones
	std::vector<serverconnection*>::iterator iter = this->connections.begin();
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
				FD_SET(currentFD, &(this->socks)); // Adds the socket file descriptor to the monitoring for select
				if (currentFD > this->highSock)
					this->highSock = currentFD; // Need the highest socket for select
			}
		}
		++iter; 
	}
}

// Clean up everything
void servercore::freeAllConnections() {
	std::vector<serverconnection*>::iterator iter = this->connections.begin();
	while (iter != this->connections.end()) {
		delete (*(iter++)); // Clean up, issue destructor implicitly
	}
	this->connections.clear(); // Delete all deleted connections also from our vector
}

// Accepts new connections and stores the connection object with fd in a vector
int servercore::handleNewConnection() {
	int fd; // Socket file descriptor for incoming connections

	this->cli_size = sizeof(this->cli);
	fd = accept(this->s, (struct sockaddr*) & cli, &cli_size);
	if (fd < 0) {
		std::cerr << "Error while accepting client" << std::endl;
		return (EXIT_FAILURE);
	}
	send(fd, "220 Welcome to my server.\r\n", 28, 0);

	// Gets the socket fd flags and add the non-blocking flag to the sfd
	this->setNonBlocking(fd);

	// Something went wrong, new connection could not be handled
	if (fd == -1) {
		std::cerr << "Something went wrong, new connection could not be handled" << std::endl;
		try {
			closesocket(fd);
		}
		catch (std::exception e) {
			std::cerr << e.what() << std::endl;
		}
		return (EXIT_FAILURE); // Return at this point
	}

	// Get the client IP address
	char ipstr[INET6_ADDRSTRLEN];
	int port;
	this->addrLength = sizeof this->addrStorage;
	getpeername(fd, (struct sockaddr*) & this->addrStorage, &(this->addrLength));
	std::string hostId = "";
	if (this->addr.sin_family == AF_INET) {
		struct sockaddr_in* fd = (struct sockaddr_in*) & (this->addrStorage);
		port = ntohs(fd->sin_port);
		inet_ntop(AF_INET, &fd->sin_addr, ipstr, sizeof ipstr);
		hostId = (std::string)ipstr;
	}

	printf("Connection accepted: FD=%d - Slot=%lu - Id=%d \n", fd, (this->connections.size() + 1), ++(this->connId));
	// The new connection (object)
	serverconnection* conn = new serverconnection(fd, this->connId, this->dir, hostId, this->commandOffset); // The connection vector
	// Add it to our list for better management / convenience
	this->connections.push_back(conn);
	return (EXIT_SUCCESS);
}

// Data ready to read at a socket, either accept a new connection or handle the incoming data over an already opened socket
void servercore::readSockets() {
	// Check our "listening" socket, and then check the sockets in connectlist
	// If a client is trying to connect() to our listening socket, select() will consider that as the socket being 'readable' and thus, if the listening socket is part of the fd_set, accept a new connection
	if (FD_ISSET(this->s, &(this->socks))) {
		if (this->handleNewConnection()) {  
			std::cout << "smth went wrong on " << __FUNCTION__ << std::endl;
			return;
		}
	}
	// Check connectlist for available data
	// Run through our sockets and check to see if anything happened with them, if so 'service' them
	for (unsigned int listnum = 0; listnum < this->connections.size(); listnum++) {
		if (FD_ISSET(this->connections.at(listnum)->getFD(), &(this->socks))) {
			this->connections.at(listnum)->respondToQuery(); // Now that data is available
		}
	}
}

// Server entry point and main loop accepting and handling connections
int servercore::start() {
	struct timeval timeout; // Timeout for select
	int readsocks; // Number of sockets ready for reading
	timeout.tv_sec = 1; // Timeout = 1 sec
	timeout.tv_usec = 0;
	// Wait for connections, main server loop
	while (!this->shutdown) {

		this->buildSelectList(); // Clear out data handled in the previous iteration, clear closed sockets

		// Multiplexes between the existing connections regarding to data waiting to be processed on that connection
		readsocks = select(this->highSock + 1, &(this->socks), (fd_set*)0, (fd_set*)0, &timeout);

		if (readsocks < 0) {
			std::cerr << "Error calling select" << std::endl;
			return (EXIT_FAILURE);
		}

		this->readSockets(); // Handle the sockets (accept new connections or handle incoming data or do nothing [if no data])
	}
	return (EXIT_SUCCESS);
}

// Sets the given socket to non-blocking mode
void servercore::setNonBlocking(int& sock) {

	BOOL l = TRUE;
	if (SOCKET_ERROR == ioctlsocket(sock, FIONBIO, (unsigned long*)&l))
	{
		std::cerr << "Error setting socket to non-blocking" << std::endl;
		int res = WSAGetLastError();
		
	}
	return;
}

// Initialization of sockets / socket list with options and error checking
void servercore::initSockets(int port) {
	int reuseAllowed = 0;
	this->maxConnectionsInQuery = 50;
	this->addr.sin_family = AF_INET; 
	this->addr.sin_port = htons(port);
	this->addr.sin_addr.s_addr = INADDR_ANY; // Server can be connected to from any host
	this->s = socket(AF_INET, SOCK_STREAM, 0); 
	std::cout << "socket " << this->s << " initialised" << std::endl;

	if (this->s == -1) {
		std::cerr << "socket() failed (1) " << std::endl;
		return;
	}
	else if (setsockopt(this->s, SOL_SOCKET, SO_REUSEADDR, (const char*)(&reuseAllowed), sizeof(reuseAllowed)) < 0) { //  enable reuse of socket, even when it is still occupied
		std::cerr << "setsockopt() failed (2)" << std::endl;
		closesocket(this->s);
		return;
	}
	this->setNonBlocking(this->s);
	if (bind(this->s, (struct sockaddr*) & addr, sizeof(addr)) == -1) {
		std::cerr << ("bind() failed (3)") << std::endl;
		closesocket(this->s);
		return;
	} // 2nd parameter (backlog): number of connections in query, can be also set SOMAXCONN
	else if (listen(this->s, this->maxConnectionsInQuery) == -1) {
		std::cerr << ("listen () failed (4)") << std::endl;
		closesocket(this->s);
		return;
	}
	this->highSock = this->s; // This is the first (and the main listening) socket
	std::cout << "Server started and listening at port " << port << ", default server directory '" << this->dir << std::endl;
}
