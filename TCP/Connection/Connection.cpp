#include "Connection.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdexcept>
#include "../Utility/Utility.h"

#include <iostream>

TCP::TcpBuffer::TcpBuffer(size_t size) : 
	data(std::make_unique<char[]>(size)), 
	size(size) {}

namespace {
	void wsaInit() {
		WSADATA wsaData;
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
	wsaInit();
	useAddress(addr, port);
}

TCP::Connection::Connection(int sockfd) : sockfd(sockfd) {
	wsaInit();
}

TCP::Connection::~Connection() {
	closesocket(sockfd);
	WSACleanup();
}

namespace {
	// Basic reading, not final
	void readLoop(TCP::Connection* con, std::function<void(TCP::Connection& con, TCP::TcpBuffer& msg)> msgHandler) {
		while (true) {
			TCP::TcpBuffer buffer(2048);
			int outSize = recv(con->sockfd, buffer.data.get(), 2048, 0);
			// outSize == 0 means we've lost connection, abort loop so we can deconstruct gracefully
			if (outSize == 0) break;
			if (outSize < 0) {
				std::cout << ("Error while reading data: " + getErrorText()).c_str() << '\n';
				break;
			}
			// We can truncate the buffer by reducing the size we say it is, it's up to the client to respect the size
			buffer.size = outSize;
			msgHandler(*con, buffer);
		}
	}
}

void TCP::Connection::readMsgs(std::function<void(Connection& con, TcpBuffer& msg)> msgHandler) {
	// Using seperate function to be able to more easily add threading in the future
	readLoop(this, msgHandler);
}

void TCP::Connection::send(TcpBuffer& msg) {
	int errCount = 0;
	for (size_t bytesSent = 0; bytesSent < msg.size;) {
		int bytesSentBySend = ::send(sockfd, msg.data.get(), msg.size, 0);

		if (bytesSentBySend == -1) {
			if (++errCount == 5) {
				throw std::exception(("Failed to send TCP data 5 times in a row: " + getErrorText()).c_str());
			}
			continue;
		} else errCount = 0;

		bytesSent += bytesSentBySend;
	}
}
