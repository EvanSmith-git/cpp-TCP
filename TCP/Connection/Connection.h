#pragma once
#include <string>
#include <optional>
#include <functional>
#include <WS2tcpip.h>
#include <mutex>

namespace TCP {
	struct TcpBuffer {
		// next and prev for planned ability to link TcpBuffer instances together for efficient prepending / appending
		//TcpBuffer* prev;

		uint32_t size;
		char* data;

		//TcpBuffer* next;

		// Weak smart pointer (free on destroy)
		TcpBuffer(uint32_t size);
		// Weak smart pointer (free on destroy) initialized with string
		TcpBuffer(const std::string_view str);
		// Weak smart pointer (free on destroy) initialized with string, null filled to length
		//TcpBuffer(const std::string_view str);
		// Reference to data somewhere else (doesn't copy)
		TcpBuffer(char* data, uint32_t size);
		~TcpBuffer();

	private:
		bool shouldFreeMemory;
	};

	class Connection {
	private:
		void useAddress(std::optional<const std::string_view> addr, const std::string_view port);

		std::function<void(Connection& con, const TcpBuffer& msg)> msgHandler;
		std::function<void(Connection& con)> closeHandler;
		std::function<void(Connection& con, std::exception& ex)> errHandler;

		std::mutex closedMutex;
		bool _closed = false;

	public:
		SOCKET sockfd = 0;
		bool closed();

		std::string ip;
		std::string port;

		Connection(std::optional<const std::string_view> addr, const std::string_view port);
		// When constructing with sockfd, it's assumed that the socket is already connected
		Connection(SOCKET sockfd, const sockaddr* sockAddr);
		~Connection();

		void close();

		void onMessage(std::function<void(Connection& con, const TcpBuffer& msg)> msgHandler);
		void onClose(std::function<void(Connection& con)> closeHandler);
		void onError(std::function<void(Connection& con, std::exception& ex)> errHandler);

		void read();

		void send(const TcpBuffer& msg);
	};
}
