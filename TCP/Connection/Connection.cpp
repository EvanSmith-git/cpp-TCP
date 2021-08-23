#include "Connection.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdexcept>
#include <mutex>
#include <string>
#include <optional>

namespace {
	// Count instances so we can WSAStartup() when instances go from 0 -> 1, and WSACleanup when all instances stop
	std::mutex instanceMutex; // Mutex for thread safety of instanceCount...
	int instanceCount = 0;

	void modifyInstanceCount(int amount) {
		std::lock_guard<std::mutex> guard(instanceMutex);
		instanceCount += amount;

		WSADATA wsaData;
		if (instanceCount == 0) WSACleanup();
		else if (instanceCount == 1)
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw std::exception("WSAStartup failed");
	}
}

// Takes in port/address, sets socket descriptor
void TCP::Connection::useAddress(std::optional<std::string> addr, std::string port) {
	// Helper bools - if addr is defined we're setting up for a client
	//                otherwise we're setting up for a server
	// Client/Server could have seperate functions, but they share 90% of their code
	bool isClient = addr.has_value();
	bool isServer = !isClient;

	// Setup hints...
	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (isServer) hints.ai_flags = AI_PASSIVE;

	// Get address info for port/address
	addrinfo* servinfo;
	if (getaddrinfo(isClient ? addr.value().c_str() : NULL, port.c_str(), &hints, &servinfo) != 0)
		throw std::exception("Failed to get address info");

	// Loop through all the results and connect to the first we can
	addrinfo* p;
	int _sockfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;

		if (isServer) {
			char yes = '1';
			if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
				freeaddrinfo(servinfo);
				throw std::exception("Setsockopt failed");
			}
		}

		if ((isClient ? connect : bind)(_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(_sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		freeaddrinfo(servinfo);
		throw std::exception("Failed to connect/bind");
	}

	freeaddrinfo(servinfo);

	if (isServer && listen(_sockfd, 10) == -1)
		throw std::exception("Failed to listen");

	sockfd = _sockfd;
}

TCP::Connection::Connection(std::optional<std::string> addr, std::string port) {
	modifyInstanceCount(1);
	useAddress(addr, port);
}

TCP::Connection::Connection(int sockfd) : sockfd(sockfd) {
	modifyInstanceCount(1);
}

TCP::Connection::~Connection() {
	closesocket(sockfd);
	modifyInstanceCount(-1);
}

void TCP::Connection::readMsgs(std::function<void(Connection* con, tcpData msg)> msgHandler) {
	// TODO
}

void TCP::Connection::send(const tcpData& msg) {
	int errCount = 0;
	for (size_t bytesSent = 0; bytesSent < msg.size;) {
		int bytesSentBySend = ::send(sockfd, msg.data.get(), msg.size, 0);

		if (bytesSentBySend == -1) {
			if (++errCount == 5) {
				throw std::exception("Failed to send TCP data 5 times in a row");
			}
			continue;
		} else errCount = 0;

		bytesSent += bytesSentBySend;
	}
}