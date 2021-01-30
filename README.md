# Ftp Server
This is a simple implementation of FTP server in C++.
It comes without any warranties of any kind.

## In a few words

* Written in C++ for Windows.
* It supports only basic commands.
* You can login using any username and password.
* It supports only binary mode. (TYPE I)
* It supports only ACTIVE mode.
* It doesn't support unicode.

## How to use

* Download the project and open the solution file (simple-ftp-server.sln) in the MS Visual Studio.
* In the main file (simple-ftp-server.sln.cpp), there is an example of using the server.
* You can change some arguments at lines 12-13:\
  ```unsigned int port = smth; std::string dir = "smth";```\
  Where
  	* port - port that users will use to connect to the server
  	* dir - directory that the server will share to users
* Compile and run.
