#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_HOSTNAME_LEN 256
#define MAX_IP_LEN 46

struct HostInfo {
    std::string hostname;
    std::string ip;
};

std::mutex mtx;

void resolve_hostname(HostInfo& host_info) {
    struct addrinfo hints, * res;
    struct sockaddr_in* addr;
    int result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4

    result = getaddrinfo(host_info.hostname.c_str(), NULL, &hints, &res);
    if (result != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(result) << std::endl;
        host_info.ip = "N/A";
        return;
    }

    addr = (struct sockaddr_in*)res->ai_addr;
    char ip_str[MAX_IP_LEN];
    InetNtopA(AF_INET, &(addr->sin_addr), ip_str, MAX_IP_LEN);
    host_info.ip = ip_str;

    freeaddrinfo(res);
}

int main() {
    WSADATA wsaData;
    int wsaStartupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaStartupResult != 0) {
        std::cerr << "WSAStartup failed: " << wsaStartupResult << std::endl;
        return 1;
    }

    std::ifstream input_file("pcname.txt");
    std::ofstream output_file("pcip.txt");
    if (!input_file.is_open() || !output_file.is_open()) {
        std::cerr << "Error opening file" << std::endl;
        return 1;
    }

    std::vector<HostInfo> host_info_list;
    std::string hostname;

    // Read hostnames from file
    while (std::getline(input_file, hostname)) {
        host_info_list.push_back({ hostname, "" });
    }

    std::vector<std::thread> threads;
    auto start = std::chrono::high_resolution_clock::now();

    // Create threads to resolve hostnames
    for (auto& host_info : host_info_list) {
        threads.emplace_back(resolve_hostname, std::ref(host_info));
    }

    // Join all threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time spent: " << elapsed.count() << " seconds" << std::endl;

    // Write results to output file
    for (const auto& host_info : host_info_list) {
        output_file << host_info.hostname << " " << host_info.ip << std::endl;
    }

    input_file.close();
    output_file.close();

    WSACleanup();
    return 0;
}
