#pragma once
#include <string>
#include <optional>
#include <functional>
#include <WS2tcpip.h>

namespace TCP {
	struct TcpBuffer {
		std::unique_ptr<char[]> data;
		size_t size;
		TcpBuffer(size_t size);
	};

	class Connection {
	private:
		void useAddress(std::optional<std::string> addr, std::string port);

	public:
		int sockfd = 0;
		bool closed = false;

		std::string ip;
		std::string port;

		Connection(std::optional<std::string> addr, std::string port);
		// When constructing with sockfd, it's assumed that the socket is already connected
		Connection(int sockfd, const sockaddr* sockAddr);
		~Connection();

		void close();

		void readMsgs(std::function<void(Connection& con, TcpBuffer& msg)> msgHandler);
		void send(TcpBuffer& msg);
	};
}
