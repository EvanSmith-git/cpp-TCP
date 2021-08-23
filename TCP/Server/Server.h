#pragma once
#include "../Connection/Connection.h"
#include <functional>

namespace TCP {
	class Server : Connection {
	public:
		Server(std::string port, std::function<void(Connection* sockfd, std::string ip)> conHandler);
	};
}
