// $Id$

#include "ServerList.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#ifdef _WIN32
#include <fstream>
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <pwd.h>
#endif

using std::string;
using std::vector;

class ReadDir
{
public:
	ReadDir(const std::string& directory)
	{
		dir = opendir(directory.c_str());
	}
	~ReadDir()
	{
		if (dir) {
			closedir(dir);
		}
	}

	dirent* getEntry()
	{
		if (!dir) {
			return 0;
		}
		return readdir(dir);
	}

private:
	DIR* dir;
};

static string getTempDir()
{
	const char* result = NULL;
	if (!result) result = getenv("TMPDIR");
	if (!result) result = getenv("TMP");
	if (!result) result = getenv("TEMP");
	if (!result) {
#ifdef _WIN32
		result = "C:/WINDOWS/TEMP";
#else
		result = "/tmp";
#endif
	}
	return result;
}

static string getUserName()
{
#ifdef _WIN32
	return "default";
	// does the code below compile on win32?
	// is there a way to get the name of the current user?
#else
	struct passwd* pw = getpwuid(getuid());
	return pw->pw_name ? pw->pw_name : "";
#endif
}


static bool checkSocketDir(const string& dir)
{
	struct stat st;
	if (stat(dir.c_str(), &st)) {
		// cannot stat
		return false;
	}
	if (!S_ISDIR(st.st_mode)) {
		// not a directory
		return false;
	}
#ifndef _WIN32
	// only do permission and owner checks on *nix
	if ((st.st_mode & 0777) != 0700) {
		// wrong permissions
		return false;
	}
	if (st.st_uid != getuid()) {
		// wrong uid
		return false;
	}
#endif
	return true;
}
  
static bool checkSocket(const string& socket)
{
	string dir  = socket.substr(0, socket.find_last_of('/'));
	string name = socket.substr(socket.find_last_of('/') + 1);

	if (name.substr(0, 7) != "socket.") {
		// wrong name
		return false;
	}

	struct stat st;
	if (stat(socket.c_str(), &st)) {
		// cannot stat
		return false;
	}
#ifdef _WIN32
	if (!S_ISREG(st.st_mode)) {
		// not a regular file
		return false;
	}
#else
	if (!S_ISSOCK(st.st_mode)) {
		// not a socket
		return false;
	}
#endif
#ifndef _WIN32
	// only do permission and owner checks on *nix
	if ((st.st_mode & 0777) != 0600) {
		// check will be different on win32 (!= 777) thus actually useless 
		// wrong permissions
		return false;
	}
	if (st.st_uid != getuid()) {
		// does this work on win32? is this check meaningful?
		// wrong uid
		return false;
	}
#endif
	return true;
}

static void deleteSocket(const string& socket)
{
	unlink(socket.c_str()); // ignore errors
	string dir = socket.substr(0, socket.find_last_of('/'));
	rmdir(dir.c_str()); // ignore errors
}

int openSocket(const string& socketName)
{
	if (!checkSocket(socketName)) {
		return -1;
	}

#ifdef _WIN32
	int port = -1;
	std::ifstream in(socketName.c_str());
	in >> port;
	if (port == -1) {
		return -1;
	}

	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		return -1;
	}

	sockaddr_in addr;
	memset((char*)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(port);

#else
	int sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd == -1) {
		return -1;
	}

	sockaddr_un addr; 
	addr.sun_family = AF_UNIX; 
	strcpy(addr.sun_path, socketName.c_str()); 
#endif

	if (connect(sd, (sockaddr*)&addr, sizeof(addr)) == -1) {
		// It appears to be a socket but we cannot connect to it.
		// Must be a stale socket. Try to clean it up.
		deleteSocket(socketName);
		close(sd);
		return -1;
	}
	return sd;
}

void collectServers(vector<string>& servers)
{
	string dir = getTempDir() + "/openmsx-" + getUserName();
	if (!checkSocketDir(dir)) {
		return;
	}
	ReadDir readDir(dir);
	while (dirent* entry = readDir.getEntry()) {
		string socketName = dir + '/' + entry->d_name;
		int sd = openSocket(socketName);
		if (sd != -1) {
			close(sd);
			servers.push_back(socketName);
		}
	}
}

/*
int main()
{
#ifdef _WIN32
	WSAData wsaData;
	WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif

	vector<string> servers;
	collectServers(servers);
	
	for (vector<string>::const_iterator it = servers.begin();
	     it != servers.end(); ++it) {
		std::cout << *it << std::endl;
	}
}
*/
