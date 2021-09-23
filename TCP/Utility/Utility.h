#pragma once
#include <string>

std::string getErrorText(int errCode = 0);
std::string ipFromAddrinfo(const sockaddr* p);
std::string portFromAddrinfo(const sockaddr* p);
