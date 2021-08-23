#include "Server.h"

TCP::Server::Server(std::string port, std::function<void(Connection* sockfd, std::string ip)> conHandler) : Connection(std::nullopt, port) {
	// TODO, need to accept() from Connection sockfd and then pass those to conHandler
}
