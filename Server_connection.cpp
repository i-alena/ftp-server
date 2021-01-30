#include "Server_connection.h"

// Destructor
serverconnection::~serverconnection() {
	std::cout << "Connection terminated to client (connection id " << this->connectionId << ")" << std::endl;
	delete this->fo;
	closesocket(this->fd);
	this->directories.clear();
	this->files.clear();
}

// Constructor
serverconnection::serverconnection(int filedescriptor, unsigned int connId, std::string defaultDir, std::string hostId, unsigned short commandOffset) : fd(filedescriptor), connectionId(connId), dir(defaultDir), hostAddress(hostId), commandOffset(commandOffset), closureRequested(false), uploadCommand(false), downloadCommand(false), receivedPart(0), parameter("") {
	this->fo = new fileoperator(this->dir); // File and directory browser
	std::cout << "Connection to client '" << this->hostAddress << "' established" << std::endl;
}

// Check for matching (commands/strings) with compare method
bool serverconnection::commandEquals(std::string a, std::string b) {
	// Convert to lower case for case-insensitive checking
	std::transform(a.begin(), a.end(), a.begin(), tolower);
	int found = a.find(b);
	return (found != std::string::npos);
}

// Command switch for the issued client command, only called when this->command is set to 0
std::string serverconnection::commandParser(std::string command) {
	msg = command;
	this->parameter;
	std::string res = "";
	this->uploadCommand = false;
	struct stat Status;
	// Commands can have either 0 or 1 parameters, e.g. 'browse' or 'browse ./'
	std::vector<std::string> commandAndParameter = this->extractParameters(command);
	std::cout << "Connection " << this->connectionId << ": " << std::endl;

	std::cout << "Input command and params:" << std::endl;
	for (auto i : commandAndParameter)
		std::cout << i << " ";
	std::cout << "commandAndParameter.size" << commandAndParameter.size() << std::endl;

	// If command with no argument was issued
	if (commandAndParameter.size() == 1) {
		if (this->commandEquals(commandAndParameter.at(0), "list")) {
			// dir to browse
			std::string curDir = "./";
			// Make sure PORT has been called
			if (data_socket == INVALID_SOCKET)
			{
				// Port hasn't been set
				to_send = "451 Requested action aborted: local error in processing\n";
				std::cout << (to_send);
				send(fd, to_send.c_str(), to_send.size(), 0);
				return "";
			}

			std::string sting_of_files = "";

			this->directories.clear();
			this->files.clear();
			this->fo->browse(curDir, directories, files);


			for (unsigned int j = 0; j < directories.size(); j++) {
				sting_of_files += "d--------- 1 ftp ftp              0 " + directories.at(j) + "/\r\n";
			}
			for (unsigned int i = 0; i < files.size(); i++) {
				sting_of_files += "---------- 1 ftp ftp              0 " + files.at(i) + "\r\n";
			}

			//tell the client that data transfer is about to happen
			to_send = "125 Data connection already open; transfer starting. \r\n";
			std::cout << (to_send);
			send(fd, to_send.c_str(), to_send.size(), 0);

			// send
			send(data_socket, sting_of_files.c_str(), sting_of_files.length(), 0);
			std::cout << ("Files list:\n" + sting_of_files);
			closesocket(data_socket);
			data_socket = INVALID_SOCKET;

			// done
			to_send = "250 Requested file action okay, completed.\n";

			std::cout << (to_send);

			send(fd, to_send.c_str(), to_send.size(), 0);
		}
		else
			if (this->commandEquals(commandAndParameter.at(0), "syst")) {
				res = "215 WINDOWS-NT-10.0";
			}
			else
				if (this->commandEquals(commandAndParameter.at(0), "feat")) {
					res = "211 End";
				}
				else
					if (this->commandEquals(commandAndParameter.at(0), "pwd")) { // Returns the current working directory on the server
						std::cout << "Working dir requested" << std::endl;
						res = "257 Current Directory: \"" + this->full_path + "\"";
					}
					else
						if (this->commandEquals(commandAndParameter.at(0), "getparentdir")) { // Returns the parent directory of the current working directory on the server
							std::cout << "Parent dir of working dir requested" << std::endl;
							res = this->fo->getParentDir();
						}
						else
							if (this->commandEquals(commandAndParameter.at(0), "cdup")) {
								parameter = "..";
								std::cout << "Change of working dir to '" << this->parameter << "' requested" << std::endl;
								// Test if dir exists

								if (!this->fo->changeDir(this->parameter)) {
									std::cout << "Directory change to '" << this->parameter << "' successful!" << std::endl;
								}
								res = "250 CWD successfully";
							}
							else
								if (this->commandEquals(commandAndParameter.at(0), "bye") || this->commandEquals(commandAndParameter.at(0), "quit")) {
									std::cout << "Shutdown of connection requested" << std::endl;
									this->closureRequested = true;
								}
								else {
									// Unknown / no command / enter
									std::cout << "Unknown command encountered!" << std::endl;
									commandAndParameter.clear();
								}
	}
	else { // end of commands with no arguments
		// Command with a parameter received
		if (commandAndParameter.size() > 1) {
			// The parameter
			this->parameter = commandAndParameter.at(1);

			if (this->commandEquals(commandAndParameter.at(0), "type")) {
				if (this->parameter == "I") {
					res = "501 ONLY bins here";
				}
				else {
					res = "202 JUST WORK";
				}
			}
			else

				if (this->commandEquals(commandAndParameter.at(0), "port")) {
					// open data port
					int c2 = msg.rfind(",");
					int c1 = msg.rfind(",", c2 - 1);

					// Calculate port number
					int port_number = atoi(msg.substr(c1 + 1, c2 - c1).c_str()) * 256;
					port_number += atoi(msg.substr(c2 + 1).c_str());

					// Get address
					std::string address = msg.substr(5, c1 - 5);
					for (int i = 0; i < address.size(); i++)
					{
						if (address.substr(i, 1) == ",")
						{
							address.replace(i, 1, ".");
						}
					}

					std::cout << "Attempting to open port: " + std::to_string(port_number) + "\n";
					std::cout << "From address: " + address + "\n";

					data_socket = get_socket(port_number, false, address);

					if (data_socket == INVALID_SOCKET)
					{
						// Port open failure
						to_send = "425 Can't open data connection.\n";
						std::cout << (to_send);
						send(fd, to_send.c_str(), to_send.size(), 0);
						return "";
					}

					// Port open success
					to_send = "200 Command okay.\n";
					std::cout << (to_send);
					send(fd, to_send.c_str(), to_send.size(), 0);
				}
				else
					if (this->commandEquals(commandAndParameter.at(0), "list")) {


						// Make sure PORT has been called
						if (data_socket == INVALID_SOCKET)
						{
							// Port hasn't been set
							to_send = "451 Requested action aborted: local error in processing\n";
							std::cout << (to_send);
							send(fd, to_send.c_str(), to_send.size(), 0);
							return "";
						}

						std::string sting_of_files = "";


						// read out dir to browse
						std::string curDir = std::string(commandAndParameter.at(1));
						this->directories.clear();
						this->files.clear();
						this->fo->browse(curDir, directories, files);
						for (unsigned int j = 0; j < directories.size(); j++) {
							sting_of_files += "drwxr-xr-x 1 owner group\015\012 " + directories.at(j) + "/\n";
						}
						for (unsigned int i = 0; i < files.size(); i++) {
							sting_of_files += "-rw-r--r-- 1 owner group\015\012 " + files.at(i) + "\n";
						}

						//tell the client that data transfer is about to happen
						to_send = "125 Data connection already open; transfer starting. \r\n";
						std::cout << (to_send);
						send(fd, to_send.c_str(), to_send.size(), 0);

						// send
						send(data_socket, sting_of_files.c_str(), sting_of_files.length(), 0);
						std::cout << ("Files list:\n" + sting_of_files);
						closesocket(data_socket);
						data_socket = INVALID_SOCKET;

						// done
						to_send = "250 Requested file action okay, completed.\n";

						std::cout << (to_send);

						send(fd, to_send.c_str(), to_send.size(), 0);


					}
					else
						if (this->commandEquals(commandAndParameter.at(0), "user")) {
							std::cout << "User name get" << std::endl;
							res = "331 Need pass";
						}
						else
							if (this->commandEquals(commandAndParameter.at(0), "pass")) {
								std::cout << "Pass get" << std::endl;
								res = "230 Any pass OK";
							}
							else
								if (this->commandEquals(commandAndParameter.at(0), "retr")) {
									parameter.pop_back();
									parameter.pop_back();

									const int DATABUFLEN = 1024;


									std::string fullname = full_path;
									fullname += parameter;

									std::cout << "fullname: " << fullname << "\n";


									FILE* ifile = fopen(fullname.c_str(), "rb");
									if (!ifile) {
										std::cout << "fail to open the file!\n";
										return ("550 Failed!");

									}
									to_send = "150 ready to transfer. \r\n";
									std::cout << (to_send);
									send(fd, to_send.c_str(), to_send.size(), 0);

									char databuf[DATABUFLEN];
									int count;
									while (!feof(ifile))
									{
										count = fread(databuf, 1, DATABUFLEN, ifile);
										send(data_socket, databuf, count, 0);
									}
									memset(databuf, 0, DATABUFLEN);
									fclose(ifile);
									closesocket(data_socket);
									res = ("226 transfer complete.");

								}
								else
									if (this->commandEquals(commandAndParameter.at(0), "stor")) {
										parameter.pop_back();
										parameter.pop_back();
										const int DATABUFLEN = 1024;

										std::string dstFilename = full_path;
										dstFilename += parameter;
										std::cout << "dstFilename: " << dstFilename << "\n";


										std::ofstream ofile;
										ofile.open(dstFilename, std::ios_base::binary);

										int ret;
										char tempbuf[DATABUFLEN + 1];
										to_send = ("150 OK to send data.\r\n");
										send(fd, to_send.c_str(), to_send.size(), 0);


										ret = recv(data_socket, tempbuf, DATABUFLEN, 0);
										while (ret > 0) {
											ofile.write(tempbuf, ret);
											ret = recv(data_socket, tempbuf, DATABUFLEN, 0);
										}
										ofile.close();
										closesocket(data_socket);

										res = "226 transfer complete.\r\n";

									}
									else
										if (this->commandEquals(commandAndParameter.at(0), "cwd")) { // Changes the current working directory on the server


											std::cout << "Change of working dir to '" << this->parameter << "' requested" << std::endl;
											// Test if dir exists

											parameter.pop_back();
											parameter.pop_back();

											if (this->parameter[1] == ':') {
												parameter.erase(parameter.begin(), parameter.begin() + this->full_path.size());
												std::cout << "PATH after erase: " << parameter << std::endl;
											}
											if (!this->fo->changeDir(this->parameter)) {
												std::cout << "Directory change to '" << this->parameter << "' successful!" << std::endl;
											}
											std::cout << "Full_path: \n";
											full_path = "";
											for (auto i : this->fo->completePath) {
												full_path += i;
											}
											std::cout << full_path << std::endl;
											res = "250 CWD successfully"; // Return current directory on the server to the client (same as pwd)
										}
										else
											if (this->commandEquals(commandAndParameter.at(0), "rmdir")) {
												std::cout << "Deletion of dir '" << this->parameter << "' requested" << std::endl;
												if (this->fo->dirIsBelowServerRoot(this->parameter)) {
													std::cerr << "Attempt to delete directory beyond server root (prohibited)" << std::endl;
													res = "//"; // Return some garbage as error indication :)
												}
												else {
													this->directories.clear(); // Reuse directories to spare memory
													this->fo->clearListOfDeletedDirectories();
													this->files.clear(); // Reuse files to spare memory
													this->fo->clearListOfDeletedFiles();
													if (this->fo->deleteDirectory(this->parameter)) {
														std::cerr << "Error when trying to delete directory '" << this->parameter << "'" << std::endl;
													}
													this->directories = this->fo->getListOfDeletedDirectories();
													this->files = this->fo->getListOfDeletedFiles();
													for (unsigned int j = 0; j < directories.size(); j++) {
														res += directories.at(j) + "\n";
													}
													for (unsigned int i = 0; i < files.size(); i++) {
														res += files.at(i) + "\n";
													}
												}
											}
											else
												if (this->commandEquals(commandAndParameter.at(0), "dele")) {
													parameter.pop_back();
													parameter.pop_back();
													std::cout << "Deletion of file '" << this->parameter << "' requested" << std::endl;
													this->fo->clearListOfDeletedFiles();
													if (this->fo->deleteFile(this->parameter)) {
														res = "250 //";
													}
													else {
														std::vector<std::string> deletedFile = this->fo->getListOfDeletedFiles();
														if (deletedFile.size() > 0)
															res = "250 Dele file " + deletedFile.at(0);
													}
												}
												else
													if (this->commandEquals(commandAndParameter.at(0), "mkdir")) { // Creates a directory of the specified name in the current server working dir
														std::cout << "Creating of dir '" << this->parameter << "' requested" << std::endl;
														res = (this->fo->createDirectory(this->parameter) ? "//" : this->parameter); // return "//" in case of failure
													}
													else
														if (this->commandEquals(commandAndParameter.at(0), "touch")) { // Creates an empty file of the specified name in the current server working dir
															std::cout << "Creating of empty file '" << this->parameter << "' requested" << std::endl;
															res = (this->fo->createFile(this->parameter) ? "//" : this->parameter);  // return "//" in case of failure
														}
														else {
															// Unknown / no command
															std::cout << "Unknown command encountered! (no command)" << std::endl;
															commandAndParameter.clear();
															command = "";
															res = "ERROR: Unknown command! (no command)";
														}
		}
		else // end of command with one parameter
			// No command / enter
			if (!commandAndParameter.at(0).empty()) {
				std::cout << "Unknown command encountered! (no command)" << std::endl;
				std::cout << std::endl;
				commandAndParameter.clear();
			}
	}
	res += "\r\n";
	return res;
}

// Extracts the command and parameter (if existent) from the client call
std::vector<std::string> serverconnection::extractParameters(std::string command) {
	std::vector<std::string> res = std::vector<std::string>();
	std::size_t previouspos = 0;
	std::size_t pos;
	// First get the command by taking the string and walking from beginning to the first blank
	if ((pos = command.find(SEPARATOR, previouspos)) != std::string::npos) { // No empty string
		res.push_back(command.substr(int(previouspos), int(pos - previouspos)));
	}
	if (command.length() > (pos + 1)) {
		res.push_back(command.substr(int(pos + 1), int(command.length() - (pos + (this->commandOffset)))));
	}
	return res;
}

// Receives the incoming data and issues the apropraite commands and responds
void serverconnection::respondToQuery() {
	char buffer[BUFFER_SIZE];
	int bytes;
	bytes = recv(this->fd, buffer, sizeof(buffer), 0);
	// In non-blocking mode, bytes <= 0 does not mean a connection closure!
	if (bytes > 0) {
		std::string clientCommand = std::string(buffer, bytes);
		if (this->uploadCommand) { // (Previous) upload command
			/// Previous (upload) command continuation, store incoming data to the file
			std::cout << "Part " << ++(this->receivedPart) << ": ";
			this->fo->writeFileBlock(clientCommand);
		}
		else {
			// If not upload command issued, parse the incoming data for command and parameters
			std::string res = this->commandParser(clientCommand);
			this->sendToClient(res); // Send response to client if no binary file
		}
	}
	else { // no bytes incoming over this connection
		if (this->uploadCommand) { // If upload command was issued previously and no data is left to receive, close the file and connection
			this->uploadCommand = false;
			this->downloadCommand = false;
			this->closureRequested = true;
			this->receivedPart = 0;
		}
	}
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(char* response, unsigned long length) {
	// Sending the response
	unsigned int bytesSend = 0;
	while (bytesSend < length) {
		int ret = send(this->fd, response + bytesSend, length - bytesSend, 0);
		if (ret <= 0) {
			return;
		}
		bytesSend += ret;
	}
}

// Sends the given string to the client using the current connection
void serverconnection::sendToClient(std::string response) {
	// Sending the response
	unsigned int bytesSend = 0;
	while (bytesSend < response.length()) {
		int ret = send(this->fd, response.c_str() + bytesSend, response.length() - bytesSend, 0);
		if (ret <= 0) {
			return;
		}
		bytesSend += ret;
	}
}

// Returns the file descriptor of the current connection
int serverconnection::getFD() {
	return this->fd;
}

// Returns whether the connection was requested to be closed (by client)
bool serverconnection::getCloseRequestStatus() {
	return this->closureRequested;
}

unsigned int serverconnection::getConnectionId() {
	return this->connectionId;
}

SOCKET serverconnection::get_socket(char* s, bool toBind, std::string address)
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

		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)& yes, sizeof(int)) == -1)
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
		break;
	}
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

SOCKET serverconnection::get_socket(int p, bool toBind, std::string address)
{
	char port[6];
	sprintf(port, "%d", p);
	port[5] = '\0';
	return get_socket(port, toBind, address);
}

