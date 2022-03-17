#pragma once
#include "../Connection/Connection.h"
#include <functional>
#include <thread>
#include <mutex>

namespace TCP {
	class Server : public Connection {
		std::vector<std::thread> connectionThreads;
		std::mutex connectionsVecMutex;
		std::vector<Connection*> connections;

	public:
		Server(const std::string_view port, std::function<void(Server& self, Connection& con)> conHandler);
		~Server();
	};
}
