#pragma once

#pragma comment(lib, "Ws2_32.lib")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <direct.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <list>
#include <filesystem>
#include <stdio.h>

// Buffer size
#define BUFFER_SIZE 4096

// This contains the designation for the server root directory
#define SERVERROOTPATHSTRING ""

namespace fs = std::filesystem;

/*	The strict parameter distincts the access rights:
	strict = true:
	Only file and directory names in the current working dir are permitted as parameter,
	especially references over several directories like ../../filename are prohibited
	to ensure we do not drop under the server root directory by user command,
	used with client side commands
	strict = false:
	References over several directories like ../../filename are allowed as parameters,
	only used at server side*/


class File_operator {
public:
	std::string current_path;
	std::string root_dir;

	static bool dirExists(const std::string& dirName_in);
	File_operator(std::string dir);
	virtual ~File_operator();
	int readFile(std::string fileName);
	char* readFileBlock(unsigned long& sizeInBytes);
	int writeFileAtOnce(std::string fileName, char* content);
	int beginWriteFile(std::string fileName);
	int writeFileBlock(std::string content);
	void closeWriteFile();
	std::string changeDir(std::string newPath);;
	bool createFile(std::string& fileName, bool strict = true);
	std::string deleteFile(std::string fileName);
	std::string browse(std::string dir);
private:
	void getValidDir(std::string& dirName);
	void getValidFile(std::string& fileName);
	void stripServerRootString(std::string& dirOrFileName);
	std::ofstream currentOpenFile;
	std::ifstream currentOpenReadFile;

	static void IntToString(int i, std::string& res);
};
