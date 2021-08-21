#pragma once
#include <string>
#include <optional>
#include <memory>

struct tcpData {
	std::unique_ptr<char> data;
	size_t size;
};

namespace TCP {
	class Core {
	protected:
		Core();
		~Core();
		bool isInit = false;
		int sockfd = 0;
		void useAddress(std::string port, std::optional<std::string> addr = std::nullopt);
	};
}
