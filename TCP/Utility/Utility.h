#pragma once
#include <string>
#include <WS2tcpip.h>

std::string getErrorText(int errCode = 0);
std::string ipFromAddrinfo(const sockaddr* p);
std::string portFromAddrinfo(const sockaddr* p);
