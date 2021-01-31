#include "Server_connection.h"

// Constructor
Server_connection::Server_connection(int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset) : commandSocket(filedescriptor), connectionId(connId), defaultDir(defaultDir), hostAddress(hostId), commandOffset(commandOffset), closureRequested(false), receivedPart(0) {
	this->fo = new File_operator(this->defaultDir); // File and directory browser
	std::cout << "Connection to client '" << this->hostAddress << "' established" << std::endl;
}

// Destructor
Server_connection::~Server_connection() {
	std::cout << "Connection terminated to client (connection id " << this->connectionId << ")" << std::endl;
	delete this->fo;
	closesocket(this->commandSocket);
	this->directories.clear();
	this->files.clear();
}

// Check for matching (commands/strings) with compare method
bool Server_connection::commandEquals(std::string a, std::string b) {
	// Convert to lower case for case-insensitive checking
	std::transform(a.begin(), a.end(), a.begin(), tolower);
	int found = a.find(b);
	return (found != std::string::npos);
}

// Command switch for the issued client command, only called when this->command is set to 0
std::string Server_connection::commandParser(std::string clientCommand) {
	
	// delete endl in command
	size_t pos = 0;
	while ((pos = clientCommand.find("\r\n", pos)) != std::string::npos)
		clientCommand.replace(pos, 2, "");

	std::string res = "";

	std::vector<std::string> commandAndParameter = this->extractParameters(clientCommand);

	std::string command = commandAndParameter.at(0);
	std::string parameter = commandAndParameter.at(1);

	std::cout << "Input command: \"" << clientCommand << "\" from client " << this->connectionId << std::endl;
	// If command with no argument was issued
	std::cout << "Output: ";

	if (this->commandEquals(command, "syst")) {
		res = "215 WINDOWS-NT-10.0\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "feat")) {
		res = "211 End\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "pwd")) { // Returns the current working directory on the server
		res = "257 Current directory: \"" + fo->current_path + "\"\r\n" ;
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "list")) {
		
		this->directories.clear();
		this->files.clear();

		if (parameter == "") {
			parameter = fo->current_path;
		}
		// Now, make sure PORT has been called
		if (dataSocket == INVALID_SOCKET)
		{
			// Port hasn't been set
			res = "451 Requested action aborted: local error in processing\r\n";
			std::cout << res;
			return res;
		}

		std::string sting_of_files = fo->browse(parameter);

		//tell the client that data transfer is about to happen
		res = "125 Data connection already open; transfer starting. \r\n";
		std::cout << (res);
		send(commandSocket, res.c_str(), res.size(), 0);

		// send
		std::cout << ("Sended files list:\n" + sting_of_files + "\n");
		send(dataSocket, sting_of_files.c_str(), sting_of_files.length(), 0);
		closesocket(dataSocket);
		dataSocket = INVALID_SOCKET;

		// done
		res = "250 Requested file action okay, completed.\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "bye") || this->commandEquals(command, "quit")) {
		res = "250 Shutdown of connection requested \r\n";
		this->closureRequested = true;
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "type")) {
		if (parameter == "I") {
			res = "202 JUST WORK\r\n";
		}
		else {
			res = "501 ONLY bins here\r\n";
		}
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "port")) {
		// open data port
		
		int c2 = clientCommand.rfind(",");
		int c1 = clientCommand.rfind(",", (c2 - 1));

		// Calculate port number
		int port_number = atoi(clientCommand.substr((c1 + 1), (c2 - c1)).c_str()) * 256;
		port_number += atoi(clientCommand.substr((c2 + 1)).c_str());

		// Get address
		std::string address = clientCommand.substr(5, (c1 - 5));
		for (int i = 0; i < address.size(); i++)
		{
			if (address.substr(i, 1) == ",")
			{
				address.replace(i, 1, ".");
			}
		}

		std::cout << "Attempting to open port: " + std::to_string(port_number) << " from address: " + address + "\r\n";

		dataSocket = getSocket(port_number, false, address);

		if (dataSocket == INVALID_SOCKET)
		{
			// Port open failure
			res = "425 Can't open data connection.\r\n";
			std::cout << res;
			return res;
		}

		// Port open success
		res = "200 Command okay.\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "user")) {
		res = "331 User get. Need pass\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "pass")) {
		res = "230 Pass OK\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "retr")) {
		const int DATABUFLEN = 1024;
		

		//Get path to file
		std::string fullname = fo->current_path;
		fullname += parameter;

		FILE* ifile = fopen(fullname.c_str(), "rb");
		if (!ifile) {
			res = "550 fail to open the file!\r\n";
			std::cout << res;
			return res;
		}
		res = "150 ready to transfer. \r\n";
		std::cout << res;
		send(commandSocket, res.c_str(), res.size(), 0);

		char databuf[DATABUFLEN];
		int count;
		while (!feof(ifile))
		{
			count = fread(databuf, 1, DATABUFLEN, ifile);
			send(dataSocket, databuf, count, 0);
		}
		memset(databuf, 0, DATABUFLEN);
		fclose(ifile);
		closesocket(dataSocket);

		res = ("226 transfer complete.\r\n");
		std::cout << res;
		return res;

	}

	if (this->commandEquals(command, "stor")) {
		const int DATABUFLEN = 1024;


		//Get full path to file
		std::string dstFilename = fo->current_path;
		dstFilename += parameter;


		std::ofstream ofile;
		ofile.open(dstFilename, std::ios_base::binary);

		int ret;
		char tempbuf[DATABUFLEN + 1];
		res = ("150 OK to send data.\r\n");
		send(commandSocket, res.c_str(), res.size(), 0);


		ret = recv(dataSocket, tempbuf, DATABUFLEN, 0);
		while (ret > 0) {
			ofile.write(tempbuf, ret);
			ret = recv(dataSocket, tempbuf, DATABUFLEN, 0);
		}
		ofile.close();
		closesocket(dataSocket);

		res = "226 transfer complete.\r\n";
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "cwd")) { // Changes the current working directory on the server
		if (parameter.size() > 1) {
			if (parameter[1] != ':') { //relative path
				parameter = this->fo->current_path + parameter;
			}
		}

		res = this->fo->changeDir(parameter + "\\") ;
		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "cdup")) {
		//Delete last dir in path
		std::size_t pos_to_del = fo->current_path.find_last_of("\\");
		parameter = fo->current_path.substr(0, pos_to_del);
		pos_to_del = parameter.find_last_of("\\");
		parameter = parameter.substr(0, pos_to_del);
		parameter += "\\";
		res = fo->changeDir(parameter);
		std::cout << "CDUP path: " << parameter << std::endl;

		std::cout << res;
		return res;
	}

	if (this->commandEquals(command, "dele")) {
		res = fo->deleteFile(parameter);
		std::cout << res;
		return res;
	}

	res = "550 Unknown command \""+ clientCommand +"\"\r\n";
	commandAndParameter.clear();
	std::cout << res << std::endl;
	return res;
}

// Extracts the command and parameter (if existent) from the client call
std::vector<std::string> Server_connection::extractParameters(std::string command) {
	std::vector<std::string> res = std::vector<std::string>();
	std::size_t previouspos = 0;
	std::size_t pos;
	// First get the command by taking the string and walking from beginning to the first blank
	if ((pos = command.find(SEPARATOR, previouspos)) != std::string::npos) { // No empty string
		res.push_back(command.substr(int(previouspos), int(pos - previouspos))); // The command
	}
	if (command.length() > (pos + 1)) {
		res.push_back(command.substr(int(pos + 1), int(command.length() - (pos + (this->commandOffset)))));
	}
	while (res.size() < 2) {
		res.push_back("");
	}
	return res;
}

// Receives the incoming data and issues the apropraite commands and responds
void Server_connection::respondToQuery() {
	char buffer[BUFFER_SIZE];
	int bytes;
	bytes = recv(this->commandSocket, buffer, sizeof(buffer), 0);
	if (bytes > 0) {
		std::string clientCommand = std::string(buffer, bytes);
		// If not upload command issued, parse the incoming data for command and parameters
		std::string res = this->commandParser(clientCommand);
		if (res != "") {
			this->sendToClient(res); // Send response to client if no binary file
		}
	}
}

// Sends the given string to the client using the current connection
void Server_connection::sendToClient(std::string response) {
	// Now sending the response
	unsigned int bytesSend = 0;
	while (bytesSend < response.length()) {
		int ret = send(this->commandSocket, response.c_str() + bytesSend, response.length() - bytesSend, 0);
		if (ret <= 0) {
			return;
		}
		bytesSend += ret;
	}
}

// Returns the file descriptor of the current connection
int Server_connection::getFD() {
	return this->commandSocket;
}

// Returns whether the connection was requested to be closed (by client)
bool Server_connection::getCloseRequestStatus() {
	return this->closureRequested;
}

unsigned int Server_connection::getConnectionId() {
	return this->connectionId;
}

SOCKET Server_connection::getSocket(char* s, bool toBind, std::string address)
{
	int status;
	struct addrinfo hints;
	struct addrinfo* servinfo;
	struct addrinfo* bound;
	SOCKET sock = INVALID_SOCKET;
	int yes = 1;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//Get address info
	if (toBind)
	{
		if ((status = getaddrinfo(NULL, s, &hints, &servinfo)) != 0)
		{
			std::cout << ("Error: getaddrinfo failed.\n");
			return INVALID_SOCKET;
		}
	}
	else
	{
		if ((status = getaddrinfo(address.c_str(), s, &hints, &servinfo)) != 0)
		{
			std::cout << ("Error: getaddrinfo failed.\n");
			return INVALID_SOCKET;
		}
	}

	//Try to bind/connect

	for (bound = servinfo; bound != NULL; bound = bound->ai_next)
	{
		sock = socket(bound->ai_family, bound->ai_socktype, bound->ai_protocol);
		if (sock == INVALID_SOCKET) continue;

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int)) == -1)
		{
			std::cout << ("Error: setsockopt failed.\n");
			return INVALID_SOCKET;
		}
		if (toBind)
		{
			// try to bind
			if (bind(sock, bound->ai_addr, bound->ai_addrlen) == -1)
			{
				continue;
			}
		}
		else
		{
			//try to connect
			if (connect(sock, bound->ai_addr, bound->ai_addrlen) == -1)
			{
				continue;
			}
		}

		// If you get this far, you've successfully bound
		break;
	}

	//If you get here with a NULL bound, then you were unsuccessful
	if (bound == NULL)
	{
		std::cout << ("Error: failed to bind to anything.\n");
		return INVALID_SOCKET;
	}

	//Free up the space since the addrinfo is no longer needed
	freeaddrinfo(servinfo);

	//Try to listen if you're binding
	if (toBind)
	{
		if (listen(sock, SOMAXCONN) == -1)
		{
			std::cout << ("Error: failed to listen.\n");
			return INVALID_SOCKET;
		}
	}

	// return the fd
	return sock;
}

SOCKET Server_connection::getSocket(int p, bool toBind, std::string address)
{
	char port[6];
	sprintf(port, "%d", p);
	port[5] = '\0';
	return getSocket(port, toBind, address);
}
