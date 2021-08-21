#pragma once
#include <string>
#include <optional>

namespace TCP {
	class Core {
	protected:
		Core();
		~Core();
		int sockfd = 0;
		void useAddress(std::string port, std::optional<std::string> addr = std::nullopt);
	};
}
