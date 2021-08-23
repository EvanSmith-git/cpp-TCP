#pragma once
#include "../Connection/Connection.h"
#include <functional>

namespace TCP {
	class Client : Connection {
	public:
		Client(std::string addr, std::string port, std::function<void(Connection* con)> conHandler);
	};
}
