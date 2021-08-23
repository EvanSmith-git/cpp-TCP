#pragma once
#include <string>
#include <optional>
#include <memory>
#include <functional>

struct tcpData {
	std::unique_ptr<char[]> data;
	size_t size;
};

namespace TCP {
	class Connection {
	private:
		void useAddress(std::optional<std::string> addr, std::string port);

	protected:
		std::function<void(Connection* con, tcpData msg)> msgHandler;
		int sockfd = 0;

	public:
		Connection(std::optional<std::string> addr, std::string port);
		// When constructing with sockfd, it's assumed that the socket is already connected
		Connection(int sockfd);
		~Connection();

		void readMsgs(std::function<void(Connection* con, tcpData msg)> msgHandler);
		void send(const tcpData& msg);
	};
}
