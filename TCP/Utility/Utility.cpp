#include "Utility.h"
#if defined(_WIN32)
#include <Windows.h>
#endif

std::string getErrorText() {
#if defined(_WIN32)
    static char message[256] = { 0 };
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, WSAGetLastError(), 0, message, 256, 0);
    char* nl = strrchr(message, '\n');
    if (nl) *nl = 0;
    return message;

#else
    return strerror(errno);

#endif
}
