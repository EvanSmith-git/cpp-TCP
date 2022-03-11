#include "Server.h"
#include "../Utility/Utility.h"
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>

TCP::Server::Server(const std::string_view port, std::function<void(Server& self, Connection& con)> conHandler) : Connection(std::nullopt, port) {
	while (!closed()) {
		sockaddr addr;
		int addrlen = 32;
		SOCKET newfd = accept(sockfd, &addr, &addrlen);

		// When adding *nix support, error handling will have to be changed to be OS agnostic
		// Otherwise, nearly all of the other code written is already *nix compatible
		if (newfd == INVALID_SOCKET) {
			int errNo = WSAGetLastError();

			if (errNo == WSAEINTR) // Closing
				return;

			// If our socket is invalid, it must have closed
			if (errNo == WSAENOTSOCK || errNo == WSAEOPNOTSUPP) {
				close();
				break;
			}

			throw std::exception(("Server error while accepting connections: " + getErrorText(errNo)).c_str());
		}
		
		connectionThreads.push_back(std::thread([this, conHandler, newfd, addr] {
			Connection newCon(newfd, &addr);

			{
				std::lock_guard<std::mutex> guard(connectionsVecMutex);
				connections.push_back(&newCon);
			}

			conHandler(*this, newCon);
		}));
	}
}

TCP::Server::~Server() {
	// Close connections first, and then join any joinable threads
	// Seperate loops to quickly close all first, instead of waiting on threads
	std::lock_guard<std::mutex> guard(connectionsVecMutex);
	for (Connection* connection : connections)
		if (!connection->closed())
			connection->close();
	
	for (std::thread& connectionThread : connectionThreads)
		if (connectionThread.joinable())
			connectionThread.join();
}
