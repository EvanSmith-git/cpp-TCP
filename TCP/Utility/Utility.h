#pragma once
#include <string>

std::string getErrorText();
std::string ipFromAddrinfo(const sockaddr* p);
std::string portFromAddrinfo(const sockaddr* p);
