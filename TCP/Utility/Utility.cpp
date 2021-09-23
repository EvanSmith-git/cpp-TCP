#include "Utility.h"
#if defined(_WIN32)
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <Windows.h>
#include <WS2tcpip.h>
#endif

std::string getErrorText(int errCode = 0) {
#if defined(_WIN32)
    static char message[256] = { 0 };
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, errCode ? errCode : WSAGetLastError(), 0, message, 256, 0);
    char* nl = strrchr(message, '\n');
    if (nl) *nl = 0;
    return message;

#else
    return strerror(errCode ? errCode : errno);

#endif
}

std::string ipFromAddrinfo(const sockaddr* p) {
    char ipstr[INET6_ADDRSTRLEN];

    void* addrData = p->sa_family == AF_INET ?
        // IPv4
        (void*)&((sockaddr_in*)p)->sin_addr :
        // IPv6
        (void*)&((sockaddr_in6*)p)->sin6_addr;

    inet_ntop(p->sa_family, addrData, ipstr, sizeof ipstr);

    return std::string(ipstr);
}

std::string portFromAddrinfo(const sockaddr* p) {
    char ipstr[INET6_ADDRSTRLEN];

    USHORT port = p->sa_family == AF_INET ?
        // IPv4
        ((sockaddr_in*)p)->sin_port :
        // IPv6
        ((sockaddr_in6*)p)->sin6_port;

    return std::to_string(port);
}
