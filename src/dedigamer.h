#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>

struct DedigamerServer {
    std::string name;
    int currentPlayers = 0;
    int totalPlayers = 0;
    std::string map;
    std::string gametype;
    std::string uptime;
    std::string joinUrl;
};

struct DedigamerState {
    std::mutex mtx;
    std::vector<DedigamerServer> servers;
    int totalPlayers = 0;
    int totalCapacity = 0;
    bool fetched = false;
    bool fetching = false;
    std::string error;
    unsigned long lastFetchTick = 0;
};

namespace dedigamer
{
    inline DedigamerState g_state;
    inline std::atomic<bool> g_tabOpen{false};
    inline std::atomic<bool> g_running{false};
    inline std::thread g_thread;

    void Init();
    void Shutdown();
    void Refresh();
}
