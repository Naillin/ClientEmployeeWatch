#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wininet.h>
#include <iphlpapi.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "gdi32.lib")


void sendDataToServer(const std::string& data);

std::string getUserInfo();

std::vector<BYTE> captureScreenBMP();

void sendScreenshotToServer();

void startHttpListener();
