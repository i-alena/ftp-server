#include "File_operator.h"

// Constructor, gets the current server root dir as a parameter
File_operator::File_operator(std::string dir) {
	current_path = dir;
	root_dir = dir;
}

// Destructor
File_operator::~File_operator() {
	this->closeWriteFile(); // Close the file (if any is open)
}

// Relative directories only, strict
std::string File_operator::changeDir(std::string newPath) {
	if (newPath.find(root_dir) == std::string::npos && (newPath) != root_dir) {
		return "553 Directory \"" + newPath + "\" is up from ftp server root\r\n";
	}
	else { 
		std::wstring stemp = std::wstring(newPath.begin(), newPath.end());
		LPCWSTR sw = stemp.c_str();
		if (!SetCurrentDirectory(sw)) {
			return "553 Bad directory \"" + newPath + "\"\r\n";
		}
		else {
			current_path = newPath;
			return ("250 Directory changed to \"" + current_path + "\"\r\n");
		}
	}
}

// check error cases, e.g. newPath = '..//' , '/home/user/' , 'subdir' (without trailing slash), etc... and return a clean, valid string in the form 'subdir/'
void File_operator::getValidDir(std::string& dirName) {
	std::string slash = "/";
	size_t foundSlash = 0;
	while ((foundSlash = dirName.find_first_of(slash), (foundSlash)) != std::string::npos) {
		dirName.erase(foundSlash++, 1); // Remove all slashs
	}
	dirName.append(slash); 
}

//filename ../../e.txt -> e.txt , prohibit deletion out of valid environment
void File_operator::getValidFile(std::string& fileName) {
	std::string slash = "/";
	size_t foundSlash = 0;
	while ((foundSlash = fileName.find_first_of(slash), (foundSlash)) != std::string::npos) {
		fileName.erase(foundSlash++, 1); // Remove all slashs
	}
}

// Strips out the string for the server root, e.g. <root>/data/file -> data/file
void File_operator::stripServerRootString(std::string& dirOrFileName) {
	size_t foundRootString = 0;
	if ((dirOrFileName.find_first_of(SERVERROOTPATHSTRING)) == foundRootString) {
		int rootStringLength = ((std::string)SERVERROOTPATHSTRING).length();
		dirOrFileName = dirOrFileName.substr(rootStringLength, dirOrFileName.length() - rootStringLength);
	}
}

bool File_operator::dirExists(const std::string& dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}


// Read a block from the open file
char* File_operator::readFileBlock(unsigned long& sizeInBytes) {
	// get length of file
	this->currentOpenReadFile.seekg(0, std::ios::end);
	std::ifstream::pos_type size = this->currentOpenReadFile.tellg();
	sizeInBytes = (unsigned long)size;

	this->currentOpenReadFile.seekg(0, std::ios::beg);
	// allocate memory
	char* memblock = new char[size];
	// read data as a block
	this->currentOpenReadFile.read(memblock, size);
	std::cout << "Reading " << size << " Bytes" << std::endl;
	this->currentOpenReadFile.close();
	return memblock;
}

int File_operator::readFile(std::string fileName) {
	stripServerRootString(fileName);
	this->currentOpenReadFile.open(fileName.c_str(), std::ios::in | std::ios::binary); // modes for binary file
	if (this->currentOpenReadFile.fail()) {
		std::cout << "Reading file '" << fileName << "' failed!" << std::endl; 
		return (EXIT_FAILURE);
	}
	if (this->currentOpenReadFile.is_open()) {
		return (EXIT_SUCCESS);
	}
	std::cerr << "Unable to open file '" << fileName << " '" << std::endl; 
	return (EXIT_FAILURE);
}

int File_operator::beginWriteFile(std::string fileName) {
	stripServerRootString(fileName);
	this->currentOpenFile.open(fileName.c_str(), std::ios::out | std::ios::binary | std::ios::app); // output file
	if (!this->currentOpenFile) {
		std::cerr << "Cannot open output file '" << fileName << "'" << std::endl;
		return (EXIT_FAILURE);
	}
	std::cout << "Beginning writing to file '" << fileName << "'" << std::endl;
	return (EXIT_SUCCESS);
}

int File_operator::writeFileBlock(std::string content) {
	if (!this->currentOpenFile) {
		std::cerr << "Cannot write to output file" << std::endl;
		return (EXIT_FAILURE);
	}
	std::cout << "Appending to file" << std::endl;
	(this->currentOpenFile) << content;
	return (EXIT_SUCCESS);
}

// File is closed when disconnecting
void File_operator::closeWriteFile() {
	if (this->currentOpenFile.is_open()) {
		std::cout << "Closing open file" << std::endl;
		this->currentOpenFile.close();
	}
}

int File_operator::writeFileAtOnce(std::string fileName, char* content) {
	stripServerRootString(fileName);
	std::ofstream myFile(fileName.c_str(), std::ios::out | std::ios::binary); // output file
	if (!myFile) {
		std::cerr << "Cannot open output file '" << fileName << "'" << std::endl;
		return (EXIT_FAILURE);
	}
	myFile << content;
	myFile.close();
	return (EXIT_SUCCESS);
}

// Avoid touch ../file beyond server root
bool File_operator::createFile(std::string& fileName, bool strict) {
	if (strict)
		this->getValidFile(fileName); 
	try {
		std::ofstream fileout;
		fileout.open((this->current_path + fileName).c_str(), std::ios::out | std::ios::binary | std::ios::app);
		fileout.close();
	}
	catch (std::exception e) {
		std::cerr << e.what() << std::endl;
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}

// Lists all files and directories in the specified directory and returns them in a string vector
std::string File_operator::browse(std::string dir) {

	if (dir.find(root_dir) == std::string::npos && (dir) != root_dir) {
		return "553 Directory \"" + dir + "\" is up from ftp server root\r\n";
	}
	else {
		std::wstring stemp = std::wstring(dir.begin(), dir.end());
		LPCWSTR sw = stemp.c_str();
		if (!SetCurrentDirectory(sw)) {
			return "553 Bad directory \"" + dir + "\"\r\n";
		}
	}
	std::wstring stemp = std::wstring(current_path.begin(), current_path.end());
	LPCWSTR sw = stemp.c_str();
	SetCurrentDirectory(sw);

	std::string ans = "";
	for (const auto& entry : fs::directory_iterator(dir)) {
		std::string fileName{ entry.path().filename().string() };
		if (entry.is_directory()) {
			ans += "d--------- 1 ftp ftp              0 " + fileName + "\r\n";
		}
		else {
			ans += "---------- 1 ftp ftp               0 " + fileName + "\r\n";
		}
	}

	return ans;

}

 //Deletes a file on the server given by its name
 // Avoid rm ../file beyond server root!
std::string File_operator::deleteFile(std::string fileName) {

	if (remove(fileName.c_str()) != 0)
		return ("501 Can't delete\"" + fileName + "\"\r\n");
	else
		return ("250 File \"" + fileName + "\" deleted\r\n");
}

void File_operator::IntToString(int i, std::string& res) {
	std::ostringstream temp;
	temp << i;
	res = temp.str();
}



