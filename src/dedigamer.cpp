#define _CRT_SECURE_NO_WARNINGS
#include "dedigamer.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

static const int FETCH_INTERVAL_MS = 8000;

static std::string HttpGet(HINTERNET hConnect, const std::wstring& path)
{
    std::string result;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) return result;

    if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL))
    {
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
        {
            std::vector<char> buf(bytesAvailable);
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead);
            result.append(buf.data(), bytesRead);
        }
    }
    WinHttpCloseHandle(hRequest);
    return result;
}

static std::string HttpPost(HINTERNET hConnect, const std::wstring& path, const std::vector<BYTE>& body)
{
    std::string result;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) return result;

    const wchar_t* headers = L"Content-Type: application/octet-stream";
    if (WinHttpSendRequest(hRequest, headers, -1,
        (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0) &&
        WinHttpReceiveResponse(hRequest, NULL))
    {
        DWORD bytesAvailable = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0)
        {
            std::vector<char> buf(bytesAvailable);
            DWORD bytesRead = 0;
            WinHttpReadData(hRequest, buf.data(), bytesAvailable, &bytesRead);
            result.append(buf.data(), bytesRead);
        }
    }
    WinHttpCloseHandle(hRequest);
    return result;
}

// Build an Engine.IO v3 binary-encoded polling payload
static std::vector<BYTE> BuildBinaryPayload(const std::string& packet)
{
    std::vector<BYTE> payload;
    payload.push_back(0x00); // string type
    std::string lenStr = std::to_string(packet.size());
    for (char c : lenStr)
        payload.push_back(static_cast<BYTE>(c - '0'));
    payload.push_back(0xFF); // separator
    payload.insert(payload.end(), packet.begin(), packet.end());
    return payload;
}

static std::string ExtractJsonString(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    size_t end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

static int ExtractJsonInt(const std::string& json, const std::string& key)
{
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '"')) pos++;
    std::string num;
    while (pos < json.size() && (json[pos] >= '0' && json[pos] <= '9'))
        num += json[pos++];
    return num.empty() ? 0 : std::atoi(num.c_str());
}

static std::string ExtractSid(const std::string& response)
{
    return ExtractJsonString(response, "sid");
}

static std::vector<std::string> SplitJsonArray(const std::string& arr)
{
    std::vector<std::string> objects;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < arr.size(); i++)
    {
        if (arr[i] == '{')
        {
            if (depth == 0) start = i;
            depth++;
        }
        else if (arr[i] == '}')
        {
            depth--;
            if (depth == 0)
                objects.push_back(arr.substr(start, i - start + 1));
        }
    }
    return objects;
}

// Unescape \" inside a JSON string payload
static std::string UnescapeJson(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '\\' && i + 1 < s.size())
        {
            if (s[i + 1] == '"') { out += '"'; i++; }
            else if (s[i + 1] == '\\') { out += '\\'; i++; }
            else out += s[i];
        }
        else
        {
            out += s[i];
        }
    }
    return out;
}

static std::string FindServerStatsPayload(const std::string& data)
{
    // Format: 42/dedigamer,["serverStats","[{...escaped JSON...}]"]
    const std::string marker = "\"serverStats\",\"";
    size_t pos = data.find(marker);
    if (pos == std::string::npos) return "";
    pos += marker.size(); // now pointing at the first char of the escaped JSON string content

    // Walk forward to find the unescaped closing quote
    // The value is a JSON string containing escaped quotes, so we track escaping
    size_t end = pos;
    while (end < data.size())
    {
        if (data[end] == '\\') { end += 2; continue; }
        if (data[end] == '"') break;
        end++;
    }
    if (end >= data.size()) return "";

    std::string raw = data.substr(pos, end - pos);
    return UnescapeJson(raw);
}

static void DoFetch()
{
    {
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.fetching = true;
        dedigamer::g_state.error.clear();
    }

    HINTERNET hSession = WinHttpOpen(L"MW2DegenTool/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.error = "WinHTTP session failed";
        dedigamer::g_state.fetching = false;
        return;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, L"www.dedigamer.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.error = "Connection failed";
        dedigamer::g_state.fetching = false;
        return;
    }

    // Step 1: Engine.IO handshake
    std::string handshake = HttpGet(hConnect, L"/socket.io/?EIO=3&transport=polling");
    std::string sid = ExtractSid(handshake);
    if (sid.empty())
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.error = "Handshake failed (no sid)";
        dedigamer::g_state.fetching = false;
        return;
    }

    std::wstring basePath = L"/socket.io/?EIO=3&transport=polling&sid=";
    std::wstring wsid(sid.begin(), sid.end());
    std::wstring pollPath = basePath + wsid;

    // Step 2: Poll once to consume default namespace ack
    HttpGet(hConnect, pollPath);

    // Step 3: Connect to /dedigamer namespace (binary-encoded)
    auto nsPayload = BuildBinaryPayload("40/dedigamer,");
    HttpPost(hConnect, pollPath, nsPayload);

    // Step 4: Poll for data (try a few times)
    std::string serverPayload;
    for (int attempt = 0; attempt < 6 && serverPayload.empty(); attempt++)
    {
        if (!dedigamer::g_running.load()) break;
        std::string pollData = HttpGet(hConnect, pollPath);
        serverPayload = FindServerStatsPayload(pollData);
        if (serverPayload.empty())
            Sleep(300);
    }

    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (serverPayload.empty())
    {
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.error = "No server data received";
        dedigamer::g_state.fetching = false;
        dedigamer::g_state.lastFetchTick = GetTickCount();
        return;
    }

    // Parse server objects
    auto objects = SplitJsonArray(serverPayload);
    std::vector<DedigamerServer> servers;
    int totalPlayers = 0;
    int totalCapacity = 0;

    for (auto& obj : objects)
    {
        if (obj.find("\"server_name\"") == std::string::npos)
            continue;

        DedigamerServer srv;
        srv.name = ExtractJsonString(obj, "server_name");
        srv.currentPlayers = ExtractJsonInt(obj, "current_players");
        srv.totalPlayers = ExtractJsonInt(obj, "total_players");
        srv.map = ExtractJsonString(obj, "current_map");
        srv.gametype = ExtractJsonString(obj, "current_gt");
        srv.uptime = ExtractJsonString(obj, "uptime");
        srv.joinUrl = ExtractJsonString(obj, "join_lobby_url");
        totalPlayers += srv.currentPlayers;
        totalCapacity += srv.totalPlayers;
        servers.push_back(std::move(srv));
    }

    {
        std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
        dedigamer::g_state.servers = std::move(servers);
        dedigamer::g_state.totalPlayers = totalPlayers;
        dedigamer::g_state.totalCapacity = totalCapacity;
        dedigamer::g_state.fetched = true;
        dedigamer::g_state.fetching = false;
        dedigamer::g_state.lastFetchTick = GetTickCount();
    }

    std::cout << "[Dedigamer] Fetched " << totalPlayers << "/" << totalCapacity << " players" << std::endl;
}

static void WorkerThread()
{
    while (dedigamer::g_running.load())
    {
        if (dedigamer::g_tabOpen.load())
        {
            DWORD now = GetTickCount();
            DWORD last;
            {
                std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
                last = dedigamer::g_state.lastFetchTick;
            }
            if (now - last >= FETCH_INTERVAL_MS)
            {
                DoFetch();
            }
        }
        for (int i = 0; i < 10 && dedigamer::g_running.load(); i++)
            Sleep(100);
    }
}

void dedigamer::Init()
{
    g_running.store(true);
    g_thread = std::thread(WorkerThread);
}

void dedigamer::Shutdown()
{
    g_running.store(false);
    if (g_thread.joinable())
        g_thread.join();
}

void dedigamer::Refresh()
{
    std::lock_guard<std::mutex> lock(g_state.mtx);
    g_state.lastFetchTick = 0;
}
