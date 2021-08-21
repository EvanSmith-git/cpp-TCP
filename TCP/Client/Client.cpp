#include "Client.h"
#include "../Core/Core.h"

TCP::Client::Client(std::string addr, std::string port, std::function<void(tcpData)> _msgHandler) : msgHandler(_msgHandler), Core() {
	useAddress(port, addr);
	// Start receiving data...
}
