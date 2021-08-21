#pragma once
#include "../Core/Core.h"
#include <functional>

namespace TCP {
	class Client : Core {
	private:
		std::function<void(tcpData)> msgHandler;
	public:
		Client(std::string addr, std::string port, std::function<void(tcpData)> msgHandler);
	};
}
