#include <iostream>
#include "TCP/Client/Client.h"
#include "TCP/Server/Server.h"
#include <fstream>

void server() {
	// Have to do your own error handling for construction...
	try {
		// Construction of server is blocking (uses thread to accept new connections)
		TCP::Server myServer("678", [](TCP::Server& self, TCP::Connection& con) {
			std::cout << con.ip << " connected\n";

			// Echo back any messages
			con.onMessage([](TCP::Connection& con, const TCP::TcpBuffer& msg) {
				con.send(msg);
			});

			// Log when a client disconnects
			con.onClose([&self](TCP::Connection& con) {
				std::cout << con.ip << " disconnected\n";
				self.close();
			});

			// Log errors
			con.onError([](TCP::Connection& con, std::exception& ex) {
				std::cout << ex.what() << '\n';
			});

			// (Blocking) Start reading messages
			con.read();
		});
	}
	catch (std::exception ex) {
		std::cout << ex.what() << '\n';
	}
}

// Echo server example
// Expected output:
//     127.0.0.1 connected
//     Got: Hello World
//     127.0.0.1 disconnected
int main() {
	std::thread serverThread(server);
	
	// Construction of client isn't blocking
	TCP::Client myClient("127.0.0.1", "678", [](TCP::Connection& con) {
		// Send "Hell World" to the server
		TCP::TcpBuffer msg("Hello World");
		con.send(msg);

		// Log first message we get, then close connection
		con.onMessage([](TCP::Connection& con, const TCP::TcpBuffer& msg) {
			std::cout << "Got: " << msg.data << '\n';
			con.close();
		});

		// (Blocking) Start reading messages
		con.read();
	});

	serverThread.join();

	return 0;
}
