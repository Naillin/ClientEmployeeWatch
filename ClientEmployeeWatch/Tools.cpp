#include "Tools.h"
#include <fstream>

void log(const std::string& msg) {
    // Просто пишем в файл, можно добавить дату/время для удобства
    std::ofstream logFile("listener_log.txt", std::ios::app);
    logFile << msg << std::endl;
}

std::string getUserInfo() {
    char username[256];
    char computerName[256];
    DWORD size = 256;

    GetUserNameA(username, &size);
    size = 256;
    GetComputerNameA(computerName, &size);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char timeStr[100];
    sprintf_s(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // Получение MAC-адреса и IP
    std::string macAddress = "Unknown";
    std::string ipAddress = "Unknown";
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);

    if (GetAdaptersInfo(adapterInfo, &bufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;

        // MAC
        std::ostringstream macStream;
        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
            if (i != 0) macStream << "-";
            macStream << std::hex << std::setw(2) << std::setfill('0') << (int)pAdapter->Address[i];
        }
        macAddress = macStream.str();

        // IP
        ipAddress = pAdapter->IpAddressList.IpAddress.String;
    }

    std::string info = "User=" + std::string(username)
        + ";PC=" + std::string(computerName)
        + ";Time=" + std::string(timeStr)
        + ";MAC=" + macAddress
        + ";IP=" + ipAddress;

    return info;
}


std::string getMacAddress() {
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);
    std::string mac = "Unknown";

    if (GetAdaptersInfo(adapterInfo, &bufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;

        std::ostringstream macStream;
        for (UINT i = 0; i < pAdapter->AddressLength; i++) {
            if (i != 0) macStream << "-";
            macStream << std::hex << std::setw(2) << std::setfill('0')
                << std::uppercase << (int)pAdapter->Address[i];
        }
        mac = macStream.str();
    }
    return mac;
}

void sendDataToServer(const std::string& data) {
    const char* serverName = "127.0.0.1";
    INTERNET_PORT port = 1212;
    const char* objectName = "/report";

    HINTERNET hInternet = InternetOpenA(
        "WorkActivityClient",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0
    );
    if (!hInternet) return;

    HINTERNET hConnect = InternetConnectA(
        hInternet,
        serverName,
        port,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
    );

    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return;
    }

    const char* acceptTypes[] = { "*/*", NULL };
    HINTERNET hRequest = HttpOpenRequestA(
        hConnect,
        "POST",
        objectName,
        NULL,
        NULL,
        acceptTypes,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0
    );

    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL result = HttpSendRequestA(
        hRequest,
        headers.c_str(),
        headers.length(),
        (LPVOID)data.c_str(),
        data.length()
    );

    if (result) {
        // можно считать ответ сервера при необходимости
    }

    // очистка
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

std::vector<BYTE> captureScreenBMP() {
    log("Starting captureScreenBMP");

    std::vector<BYTE> buffer;

    int screenX = GetSystemMetrics(SM_CXSCREEN);
    int screenY = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreenDC = GetDC(NULL);
    if (!hScreenDC) {
        log("GetDC failed");
        return buffer;
    }

    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    if (!hMemoryDC) {
        log("CreateCompatibleDC failed");
        ReleaseDC(NULL, hScreenDC);
        return buffer;
    }

    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenX, screenY);
    if (!hBitmap) {
        log("CreateCompatibleBitmap failed");
        DeleteDC(hMemoryDC);
        ReleaseDC(NULL, hScreenDC);
        return buffer;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    BOOL b = BitBlt(hMemoryDC, 0, 0, screenX, screenY, hScreenDC, 0, 0, SRCCOPY);
    if (!b) {
        log("BitBlt failed");
    }
    SelectObject(hMemoryDC, hOldBitmap);

    BITMAP bmp;
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmp.bmWidth;
    bi.biHeight = bmp.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = ((bmp.bmWidth * 3 + 3) & ~3) * bmp.bmHeight;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD bmpSize = bi.biSizeImage;
    DWORD fileSize = bmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    std::vector<BYTE> bmpData(bmpSize);
    if (!GetDIBits(hMemoryDC, hBitmap, 0, bmp.bmHeight, bmpData.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        log("GetDIBits failed");
        bmpData.clear();
    }

    buffer.resize(fileSize);

    bmfHeader.bfType = 0x4D42; // 'BM'
    bmfHeader.bfSize = fileSize;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    memcpy(buffer.data(), &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(buffer.data() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));
    memcpy(buffer.data() + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), bmpData.data(), bmpSize);

    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    log("captureScreenBMP finished successfully, size = " + std::to_string(buffer.size()));
    return buffer;
}

void sendScreenshotToServer() {
    log("Starting sendScreenshotToServer");

    std::vector<BYTE> imageData = captureScreenBMP();
    if (imageData.empty()) {
        log("No image data to send");
        return;
    }

    const char* serverName = "127.0.0.1";
    INTERNET_PORT port = 1212;
    std::string objectPath = "/screenshot?mac=" + getMacAddress();

    HINTERNET hInternet = InternetOpenA(
        "ScreenshotClient",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0
    );
    DWORD timeout = 30000; // 30 секунд
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    if (!hInternet) {
        log("InternetOpenA failed");
        return;
    }
    log("InternetOpenA succeeded");

    HINTERNET hConnect = InternetConnectA(
        hInternet,
        serverName,
        port,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0
    );
    if (!hConnect) {
        log("InternetConnectA failed");
        InternetCloseHandle(hInternet);
        return;
    }
    log("InternetConnectA succeeded");

    const char* acceptTypes[] = { "*/*", NULL };
    HINTERNET hRequest = HttpOpenRequestA(hConnect,
        "POST",
        objectPath.c_str(),
        NULL, NULL,
        acceptTypes,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0
    );

    if (!hRequest) {
        log("HttpOpenRequestA failed");
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }
    log("HttpOpenRequestA succeeded");

    std::string headers = "Content-Type: application/octet-stream\r\n";

    BOOL result = HttpSendRequestA(
        hRequest,
        headers.c_str(),
        (DWORD)headers.length(),
        (LPVOID)imageData.data(),
        (DWORD)imageData.size()
    );

    if (!result) {
        log("HttpSendRequestA failed with error: " + std::to_string(GetLastError()));
    }
    else {
        log("HttpSendRequestA succeeded");
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    log("sendScreenshotToServer finished");
}

void startHttpListener() {
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;

    int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (res != 0) {
        log("WSAStartup failed with error: " + std::to_string(res));
        return;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9090);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        log("socket() failed with error: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return;
    }

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        log("bind() failed with error: " + std::to_string(WSAGetLastError()));
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        log("listen() failed with error: " + std::to_string(WSAGetLastError()));
        closesocket(listenSocket);
        WSACleanup();
        return;
    }

    log("Listener started on port 9090, waiting for connections...");

    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            log("accept() failed with error: " + std::to_string(WSAGetLastError()));
            continue;
        }
        log("Client connected.");

        char buffer[1024] = {};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived == SOCKET_ERROR) {
            log("recv() failed with error: " + std::to_string(WSAGetLastError()));
        }
        else if (bytesReceived == 0) {
            log("Connection closed by client.");
        }
        else {
            buffer[bytesReceived] = '\0';
            std::string request(buffer);
            log("Received request: " + request);

            if (request.find("GET /capture") != std::string::npos) {
                log("Request for /capture received.");
                sendScreenshotToServer();

                const char* httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
                send(clientSocket, httpResponse, (int)strlen(httpResponse), 0);
                log("Sent HTTP 200 OK response.");
            }
            else {
                log("Unknown request.");
                const char* httpResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
                send(clientSocket, httpResponse, (int)strlen(httpResponse), 0);
            }
        }

        closesocket(clientSocket);
        log("Client connection closed.");
    }

    closesocket(listenSocket);
    WSACleanup();
}
