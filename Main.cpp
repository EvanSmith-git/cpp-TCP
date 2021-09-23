#include <iostream>
#include "TCP/Client/Client.h"

int main() {
	TCP::Client myClient("127.0.0.1", "678", [](TCP::Connection& con){
		TCP::TcpBuffer msg(12);
		memcpy(msg.data.get(), "Hello World", 12);

		con.send(msg);

		con.readMsgs([](TCP::Connection& con, TCP::TcpBuffer& msg) {
			std::cout << "Got: " << msg.data.get() << '\n';
		});
	});

	return 0;
}
