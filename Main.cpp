#include <iostream>
#include "TCP/Client/Client.h"

int main() {
	TCP::Client myClient("127.0.0.1", "678", [](TCP::Connection* con){
		tcpData msg;
		msg.data = std::make_unique<char[]>(12);
		msg.size = 12;
		memcpy(msg.data.get(), "Hello World", 12);

		con->send(msg);
	});

	return 0;
}
