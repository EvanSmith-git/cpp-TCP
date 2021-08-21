#include "Core.h"
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

TCP::Core::Core() {
	modifyInstanceCount(1);
}

TCP::Core::~Core() {
	modifyInstanceCount(-1);
}

// Takes in port/address, sets socket descriptor
void TCP::Core::useAddress(std::string port, std::optional<std::string> addr) {
	if (isInit) throw std::exception("Tried to init TCP twice");
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
	if (getaddrinfo(isClient ? addr.value().c_str() : NULL, port.c_str(), &hints, &servinfo) != 0) {
		throw std::exception("Failed to get address info");
	}

	// Loop through all the results and connect to the first we can
	addrinfo* p;
	int _sockfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;

		if (isServer) {
			char yes = '1';
			if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
				throw std::exception("Setsockopt failed");
		}

		if ((isClient ? connect : bind)(_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(_sockfd);
			continue;
		}

		break;
	}

	if (p == NULL)
		throw std::exception("Failed to connect/bind");

	if (isServer && listen(_sockfd, 10) == -1)
		throw std::exception("Failed to listen");

	freeaddrinfo(servinfo);

	sockfd = _sockfd;
}
