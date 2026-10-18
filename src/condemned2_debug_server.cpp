// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Debug Server
//
// One request per line, one response line. Numbers are hex.
//   ping                       -> pong frames=<n>
//   read <addr> <len>          -> hex bytes
//   cstr <addr>                -> NUL terminated string at addr
//   write <addr> <hexbytes>    -> ok
//   writestr <addr> <text>     -> ok (text plus NUL)
//   alloc <size>               -> guest address of a new zeroed block
//   scan <text>                -> count and first matches (heap + image)
//   give <weapon>              -> queued
//   run <command>              -> runs a game command ("msg Player (...)") on the game thread, returns its result
//   call <addr> [r3..r8]       -> calls a guest function on the game thread, returns r3
//   filefind <path> <len> <needle,...> -> opens a game file through the engine (archives are decompressed), reads its first <len> bytes
//                              and counts case-insensitive matches of each needle
//   filestrings <path> <len> <substr> -> reads the first <len> bytes like filefind and returns the distinct text strings containing <substr>, '|' separated
//   loadbundle <path>          -> loads a resource bundle through the engine on the game thread, returns its handle

#include <rex/rex_app.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <format>
#include <sstream>
#include <vector>

#include <condemned2_cheats.h>
#include <condemned2_debug_server.h>

namespace Condemned2 {
    static constexpr uint32_t kCommandProcess = 0x82505008; //bool CCommandMgr::Process(mgr, command, sender, sender, flag)
    static constexpr uint32_t kCommandMgrPtr  = 0x8294F74C; //Global pointer to the command manager
    static constexpr uint32_t kEnginePtr      = 0x82932814; //Global pointer to the engine interface
    static constexpr uint32_t kImageMirror    = 0x10000000; //Writable view of the image

    static bool IsReadable(const void* ptr, size_t len) {
        uintptr_t cur = reinterpret_cast<uintptr_t>(ptr), end = cur + len;
        while (cur < end) {
            MEMORY_BASIC_INFORMATION mbi;
            if (!VirtualQuery(reinterpret_cast<void*>(cur), &mbi, sizeof(mbi)))
                return false;
            if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) || mbi.Protect == 0)
                return false;
            cur = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        }
        return true;
    }

    static uint32_t ParseHex(const std::string& text) {
        return static_cast<uint32_t>(std::stoul(text, nullptr, 16));
    }

    Condemned2DebugServer::Condemned2DebugServer(Condemned2Cheats* cheats) : _cheats(cheats) {}

    Condemned2DebugServer::~Condemned2DebugServer() {
        _stop = true;
        if (_listenSocket != ~static_cast<uintptr_t>(0))
            closesocket(static_cast<SOCKET>(_listenSocket));
        if (_thread.joinable())
            _thread.detach(); //Process is hard-exiting anyway.
    }

    void Condemned2DebugServer::Start() {
        char* env = nullptr;
        size_t envLen = 0;
        if (_dupenv_s(&env, &envLen, "C2_CONTROL") != 0 || !env)
            return;
        int port = std::atoi(env);
        free(env);
        if (port <= 0)
            return;
        _thread = std::thread([this, port]() { Run(port); });
    }

    void Condemned2DebugServer::Run(int port) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            return;
        SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr); //Localhost only
        int reuse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
        if (bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(listener, 1) != 0) {
            REXLOG_ERROR("[condemned2_debug] Failed to listen on 127.0.0.1:{}", port);
            closesocket(listener);
            return;
        }
        _listenSocket = static_cast<uintptr_t>(listener);
        REXLOG_INFO("[condemned2_debug] Listening on 127.0.0.1:{}", port);

        while (!_stop) {
            SOCKET client = accept(listener, nullptr, nullptr);
            if (client == INVALID_SOCKET)
                break;
            std::string buffer;
            char chunk[1024];
            int got;
            while ((got = recv(client, chunk, sizeof(chunk), 0)) > 0) {
                buffer.append(chunk, static_cast<size_t>(got));
                size_t newline;
                while ((newline = buffer.find('\n')) != std::string::npos) {
                    std::string line = buffer.substr(0, newline);
                    buffer.erase(0, newline + 1);
                    if (!line.empty() && line.back() == '\r')
                        line.pop_back();
                    std::string reply;
                    try {
                        reply = Handle(line);
                    } catch (const std::exception& e) {
                        reply = std::string("error: ") + e.what();
                    }
                    reply += "\n";
                    send(client, reply.c_str(), static_cast<int>(reply.size()), 0);
                }
            }
            closesocket(client);
        }
    }

    std::string Condemned2DebugServer::Handle(const std::string& line) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        std::string rest;
        std::getline(in, rest);
        if (!rest.empty() && rest.front() == ' ')
            rest.erase(0, 1);

        auto waitFor = [](std::future<std::string>& future) -> std::string {
            if (future.wait_for(std::chrono::seconds(120)) != std::future_status::ready)
                return "timeout (game thread isn't ticking: paused, in a menu or loading?)";
            return future.get();
        };

        if (cmd == "ping") {
            return std::format("pong frames={}", _cheats->GetFrameCount());
        }
        if (cmd == "read") {
            std::istringstream args(rest);
            std::string a, l;
            args >> a >> l;
            uint32_t address = ParseHex(a), len = ParseHex(l);
            if (len > 0x10000) return "error: max 0x10000 bytes";
            uint8_t* ptr = _cheats->Translate(address);
            if (!ptr || !IsReadable(ptr, len)) return "error: unreadable";
            std::string out;
            for (uint32_t i = 0; i < len; ++i) out += std::format("{:02x}", ptr[i]);
            return out;
        }
        if (cmd == "cstr") {
            uint8_t* ptr = _cheats->Translate(ParseHex(rest));
            if (!ptr) return "error: unreadable";
            std::string out;
            for (int i = 0; i < 0x400 && IsReadable(ptr + i, 1) && ptr[i]; ++i) out += static_cast<char>(ptr[i]);
            return out;
        }
        if (cmd == "write") {
            std::istringstream args(rest);
            std::string a, bytes;
            args >> a >> bytes;
            uint8_t* ptr = _cheats->Translate(ParseHex(a));
            if (!ptr || bytes.size() % 2) return "error: bad args";
            if (!IsReadable(ptr, bytes.size() / 2)) return "error: unwritable";
            for (size_t i = 0; i + 1 < bytes.size() + 1 && i < bytes.size(); i += 2)
                ptr[i / 2] = static_cast<uint8_t>(std::stoul(bytes.substr(i, 2), nullptr, 16));
            return "ok";
        }
        if (cmd == "writestr") {
            size_t space = rest.find(' ');
            if (space == std::string::npos) return "error: writestr <addr> <text>";
            uint8_t* ptr = _cheats->Translate(ParseHex(rest.substr(0, space)));
            std::string text = rest.substr(space + 1);
            if (!ptr || !IsReadable(ptr, text.size() + 1)) return "error: unwritable";
            std::memcpy(ptr, text.c_str(), text.size() + 1);
            return "ok";
        }
        if (cmd == "alloc") {
            uint32_t address = _cheats->Alloc(ParseHex(rest));
            return address ? std::format("{:08x}", address) : "error: alloc failed";
        }
        if (cmd == "scan") {
            if (rest.empty()) return "error: scan <text>";
            size_t count = 0;
            std::vector<uint32_t> hits;
            const struct { uint32_t start, size; } ranges[] = { { 0x40000000, 0x10000000 }, { 0x82000000, 0x1800000 } };
            for (auto& range : ranges) {
                uint8_t* start = _cheats->Translate(range.start);
                uintptr_t cur = reinterpret_cast<uintptr_t>(start), end = cur + range.size;
                while (cur < end) {
                    MEMORY_BASIC_INFORMATION mbi;
                    if (!VirtualQuery(reinterpret_cast<void*>(cur), &mbi, sizeof(mbi))) break;
                    uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
                    if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) && mbi.Protect != 0) {
                        const char* p = reinterpret_cast<const char*>(mbi.BaseAddress);
                        const char* e = p + mbi.RegionSize;
                        while (p + rest.size() <= e && (p = static_cast<const char*>(std::memchr(p, rest[0], static_cast<size_t>(e - p)))) != nullptr) {
                            if (p + rest.size() <= e && std::memcmp(p, rest.data(), rest.size()) == 0) {
                                ++count;
                                if (hits.size() < 20)
                                    hits.push_back(range.start + static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(p) - start));
                            }
                            ++p;
                        }
                    }
                    cur = regionEnd;
                }
            }
            std::string out = std::format("count={}", count);
            for (uint32_t hit : hits) out += std::format(" {:08x}", hit);
            return out;
        }
        if (cmd == "give") {
            if (rest.empty()) return "error: give <weapon>";
            _cheats->QueueWeapon(rest);
            return "queued";
        }
        if (cmd == "run") {
            if (rest.empty() || rest.size() > 0x1ff) return "error: run <command>";
            auto future = _cheats->RunOnGameThread([rest](PPCContext& ctx, uint8_t* base) {
                uint8_t* mgrPtr = g_condemned2_cheats->Translate(kCommandMgrPtr + kImageMirror);
                uint32_t mgr = rex::memory::load_and_swap<uint32_t>(mgrPtr);
                uint32_t scratch = g_condemned2_cheats->Scratch();
                std::memcpy(g_condemned2_cheats->Translate(scratch), rest.c_str(), rest.size() + 1);
                return std::format("{}", Condemned2Cheats::CallGuest(ctx, base, kCommandProcess, mgr, scratch, 0, 0) & 0xFF);
            });
            return waitFor(future);
        }
        if (cmd == "call") {
            std::istringstream args(rest);
            std::string a[7];
            for (auto& x : a) args >> x;
            if (a[0].empty()) return "error: call <addr> [r3 r4 r5 r6 r7 r8]";
            uint32_t v[7];
            for (int i = 0; i < 7; ++i) v[i] = a[i].empty() ? 0 : ParseHex(a[i]);
            auto future = _cheats->RunOnGameThread([v](PPCContext& ctx, uint8_t* base) {
                return std::format("{:08x}", Condemned2Cheats::CallGuest(ctx, base, v[0], v[1], v[2], v[3], v[4], v[5], v[6]));
            });
            return waitFor(future);
        }
        if (cmd == "filefind") {
            std::istringstream args(rest);
            std::string path, lenText, needleText;
            args >> path >> lenText >> needleText;
            if (path.empty() || lenText.empty() || needleText.empty()) return "error: filefind <path> <len> <needle,...>";
            uint32_t len = ParseHex(lenText);
            if (len > 0x400000) return "error: max 0x400000 bytes";
            std::vector<std::string> needles;
            std::istringstream ns(needleText);
            for (std::string n; std::getline(ns, n, ',');) if (!n.empty()) needles.push_back(n);
            auto future = _cheats->RunOnGameThread([path, len, needles](PPCContext& ctx, uint8_t* base) {
                static uint32_t fileBuffer = 0;
                if (!fileBuffer) fileBuffer = g_condemned2_cheats->Alloc(0x400000);
                uint32_t scratch = g_condemned2_cheats->Scratch();
                std::memcpy(g_condemned2_cheats->Translate(scratch), path.c_str(), path.size() + 1);
                uint8_t* data = g_condemned2_cheats->Translate(fileBuffer);
                std::memset(data, 0, len);
                auto load = [](uint32_t guestAddress) { return rex::memory::load_and_swap<uint32_t>(g_condemned2_cheats->Translate(guestAddress)); };
                uint32_t fileSystem = Condemned2Cheats::CallGuest(ctx, base, 0x821B5150);
                uint32_t stream = Condemned2Cheats::CallGuest(ctx, base, 0x821B4648, fileSystem, scratch, 0);
                if (!stream) return std::string("open failed");
                Condemned2Cheats::CallGuest(ctx, base, load(load(stream) + 4), stream, fileBuffer, len); //stream->Read(buffer, len)
                Condemned2Cheats::CallGuest(ctx, base, load(load(stream) + 8), stream);                  //stream->Release()
                std::string out = "ok";
                for (const std::string& needle : needles) {
                    size_t count = 0;
                    for (uint32_t i = 0; i + needle.size() <= len; ++i) {
                        size_t k = 0;
                        while (k < needle.size() && std::tolower(data[i + k]) == std::tolower(static_cast<unsigned char>(needle[k]))) ++k;
                        if (k == needle.size()) ++count;
                    }
                    out += std::format(" {}={}", needle, count);
                }
                return out;
            });
            return waitFor(future);
        }
        if (cmd == "filestrings") {
            std::istringstream args(rest);
            std::string path, lenText, substr;
            args >> path >> lenText >> substr;
            if (path.empty() || lenText.empty() || substr.empty()) return "error: filestrings <path> <len> <substr>";
            uint32_t len = ParseHex(lenText);
            if (len > 0x400000) return "error: max 0x400000 bytes";
            auto future = _cheats->RunOnGameThread([path, len, substr](PPCContext& ctx, uint8_t* base) {
                static uint32_t fileBuffer = 0;
                if (!fileBuffer) fileBuffer = g_condemned2_cheats->Alloc(0x400000);
                uint32_t scratch = g_condemned2_cheats->Scratch();
                std::memcpy(g_condemned2_cheats->Translate(scratch), path.c_str(), path.size() + 1);
                uint8_t* data = g_condemned2_cheats->Translate(fileBuffer);
                std::memset(data, 0, len);
                auto load = [](uint32_t guestAddress) { return rex::memory::load_and_swap<uint32_t>(g_condemned2_cheats->Translate(guestAddress)); };
                uint32_t fileSystem = Condemned2Cheats::CallGuest(ctx, base, 0x821B5150);
                uint32_t stream = Condemned2Cheats::CallGuest(ctx, base, 0x821B4648, fileSystem, scratch, 0);
                if (!stream) return std::string("open failed");
                Condemned2Cheats::CallGuest(ctx, base, load(load(stream) + 4), stream, fileBuffer, len);
                Condemned2Cheats::CallGuest(ctx, base, load(load(stream) + 8), stream);
                std::string needle = substr;
                for (char& c : needle) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                std::vector<std::string> found;
                uint32_t i = 0;
                while (i < len) {
                    uint32_t j = i;
                    while (j < len && data[j] >= 0x20 && data[j] < 0x7F) ++j;
                    if (j - i >= 4) {
                        std::string text(reinterpret_cast<char*>(data) + i, j - i);
                        std::string lowered = text;
                        for (char& c : lowered) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                        if (lowered.find(needle) != std::string::npos && std::find(found.begin(), found.end(), text) == found.end())
                            found.push_back(text);
                    }
                    i = j + 1;
                }
                std::string out = "ok";
                for (const std::string& text : found) out += "|" + text;
                return out;
            });
            return waitFor(future);
        }
        if (cmd == "loadbundle") {
            if (rest.empty() || rest.size() > 0x1ff) return "error: loadbundle <path>";
            auto future = _cheats->RunOnGameThread([rest](PPCContext& ctx, uint8_t* base) {
                uint32_t scratch = g_condemned2_cheats->Scratch();
                std::memcpy(g_condemned2_cheats->Translate(scratch), rest.c_str(), rest.size() + 1);
                auto load = [](uint32_t guestAddress) { return rex::memory::load_and_swap<uint32_t>(g_condemned2_cheats->Translate(guestAddress)); };
                uint32_t engine = load(kEnginePtr + kImageMirror);
                uint32_t rm = Condemned2Cheats::CallGuest(ctx, base, load(load(engine) + 0x14), engine);
                uint32_t handle = Condemned2Cheats::CallGuest(ctx, base, load(load(rm) + 0x68), rm, scratch);
                return std::format("{:08x}", handle);
            });
            return waitFor(future);
        }
        return "error: unknown command";
    }
}
