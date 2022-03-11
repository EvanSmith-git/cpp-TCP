#include "Connection.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <WS2tcpip.h>
#include <stdexcept>
#include "../Utility/Utility.h"
#include <thread>


TCP::TcpBuffer::TcpBuffer(uint32_t size) :
	data((char*)malloc(size)),
	size(size),
	shouldFreeMemory(true)
{
	if (!data)
		throw std::exception("TcpBuffer memory allocation failure");
}

TCP::TcpBuffer::TcpBuffer(const std::string_view str) :
	size(str.length() + 1),
	data((char*)malloc(size)),
	shouldFreeMemory(true)
{
	if (!data)
		throw std::exception("TcpBuffer memory allocation failure");

	// Copy in string data, then add a null terminator
	memcpy(data, str.data(), size - 1);
	memset(data + size - 1, '\0', 1);
}

TCP::TcpBuffer::TcpBuffer(char* data, uint32_t size) :
	data(data),
	size(size),
	shouldFreeMemory(false) {}

TCP::TcpBuffer::~TcpBuffer() {
	if (shouldFreeMemory)
		free(data);
}

namespace {
	void wsaInit() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			throw std::exception("WSAStartup failed");
	}
	
	const int yes = 1;
}

// Takes in port/address, sets socket descriptor
void TCP::Connection::useAddress(std::optional<const std::string_view> addr, const std::string_view port) {
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
	if (isServer)
		hints.ai_flags = AI_PASSIVE;

	// Get address info for port/address
	addrinfo* servinfo;
	if (getaddrinfo(isClient ? addr.value().data() : NULL, port.data(), &hints, &servinfo) != 0)
		throw std::exception("Failed to get address info");

	// Loop through all the results and connect to the first we can
	addrinfo* p;
	SOCKET _sockfd;
	for (p = servinfo; p != NULL; p = p->ai_next) {
		// Limit server to be IPv4, IPv6 should be implemented in the future
		if (isServer && p->ai_family != AF_INET) continue;
		// Create socket
		if ((_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;

		// Allow server to reuse port
		if (isServer && setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof yes) == -1) {
			freeaddrinfo(servinfo);
			throw std::exception("Setsockopt failed");
		}

		// Connect client or bind server to port
		if ((isClient ? connect : bind)(_sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(_sockfd);
			continue;
		}

		ip = ipFromAddrinfo(p->ai_addr);

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

TCP::Connection::Connection(std::optional<const std::string_view> addr, const std::string_view port) : port(port) {
	wsaInit();
	useAddress(addr, port);
}

TCP::Connection::Connection(SOCKET sockfd, const sockaddr* sockAddr) : sockfd(sockfd) {
	wsaInit();
	ip = ipFromAddrinfo(sockAddr);
	port = portFromAddrinfo(sockAddr);
}

TCP::Connection::~Connection() {
	try {
		if (!closed())
			close();
	}
	catch (std::exception ex) {
		if (errHandler)
			errHandler(*this, ex);
	}

	WSACleanup();
}

bool TCP::Connection::closed() {
	std::lock_guard guard(closedMutex);
	return _closed;
}

void TCP::Connection::close() {
	if (closed())
		throw std::exception("Attempted to close socket twice");

	{
		std::lock_guard guard(closedMutex);
		_closed = true;
	}

	closesocket(sockfd);
}

void TCP::Connection::onMessage(std::function<void(Connection& con, const TcpBuffer& msg)> msgHandler) {
	this->msgHandler = [this, msgHandler](Connection& con, const TcpBuffer& msg) {
		try {
			if (msgHandler)
				msgHandler(*this, msg);
		}
		catch (std::exception ex) {
			if (errHandler)
				errHandler(*this, ex);
		}
	};
}

void TCP::Connection::onClose(std::function<void(Connection& con)> closeHandler) {
	this->closeHandler = closeHandler;
}

void TCP::Connection::onError(std::function<void(Connection& con, std::exception& ex)> errHandler) {
	this->errHandler = errHandler;
}

namespace {
	// Recieve specified number of bytes. Blocks until that number of bytes can be read
	bool recvLen(SOCKET sockfd, char* buf, int len) {
		int outSize = recv(sockfd, buf, len, MSG_WAITALL);
		
		if (outSize == SOCKET_ERROR)
			throw std::exception(("Error while reading data: " + getErrorText()).c_str());

		if (outSize == 0) // Gracefully exit, connection closed
			return false;

		if (outSize != len)
			throw std::exception("Unexpectedly received less data from recv() than requested");

		return true;
	}
}

void TCP::Connection::read() {
	try {
		while (!closed()) {
			uint32_t packetSizeHeader;
			if (!recvLen(sockfd, (char*)&packetSizeHeader, 4))
				break;

			uint32_t packetSize = ntohl(packetSizeHeader);
			TCP::TcpBuffer message(packetSize);
			if (!recvLen(sockfd, message.data, packetSize))
				break;

			if (msgHandler)
				msgHandler(*this, message);
		}
	}
	catch (std::exception ex) {
		if (errHandler)
			errHandler(*this, ex);
	}

	try {
		if (closeHandler)
			closeHandler(*this);
	}
	catch (std::exception ex) {
		if (errHandler)
			errHandler(*this, ex);
	}
}

void TCP::Connection::send(const TcpBuffer& msg) {
	try {
		if (closed())
			throw std::exception("Attmped to send to a closed connection");

		int errCount = 0;
		int nMsgLen = htonl(msg.size);

		for (int bytesSent = 0; bytesSent < (int)msg.size + 4;) {
			int posInMsg = (bytesSent - 4);
			char* sendPtr = msg.data + posInMsg;
			int sendSize = msg.size - posInMsg;

			if (bytesSent < 4) {
				sendPtr = ((char*)&nMsgLen) + bytesSent;
				sendSize = 4 - bytesSent;
			}

			int bytesSentBySend = ::send(sockfd, sendPtr, sendSize, 0);

			if (bytesSentBySend == -1) {
				if (++errCount == 5)
					throw std::exception(("Failed to send TCP data 5 times in a row: " + getErrorText()).c_str());

				continue;
			}

			errCount = 0;
			bytesSent += bytesSentBySend;
		}
	}
	catch (std::exception ex) {
		if (errHandler)
			errHandler(*this, ex);
	}
}
