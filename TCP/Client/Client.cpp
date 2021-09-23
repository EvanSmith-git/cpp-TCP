#include "Client.h"
#include "../Connection/Connection.h"

TCP::Client::Client(std::string addr, std::string port, std::function<void(Connection& con)> conHandler) : Connection(addr, port) {
	// Connection constructor will start connection, so we can call the conHandler right away
	conHandler(*this);
}
