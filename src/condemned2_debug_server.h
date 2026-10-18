// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Debug Server
//
// Optional localhost-only text protocol for poking at the running game (read/write guest memory,
// call guest functions, spawn weapons...). Does nothing unless the C2_CONTROL environment variable
// holds a port number.

#pragma once

#include <atomic>
#include <thread>

namespace Condemned2 {
    class Condemned2Cheats;

    class Condemned2DebugServer {
        public:
            explicit Condemned2DebugServer(Condemned2Cheats* cheats);
            ~Condemned2DebugServer();

            //Starts listening if C2_CONTROL is set.
            void Start();

        private:
            void Run(int port);
            std::string Handle(const std::string& line);

            Condemned2Cheats* _cheats;
            std::thread _thread;
            std::atomic<bool> _stop = false;
            uintptr_t _listenSocket = ~static_cast<uintptr_t>(0);
    };
}
