// RexGlueSuite - Shared extraction safety utilities
#pragma once

#include <filesystem>
#include <string>
#include <algorithm>
#include <cstdint>

#include <optional>
#include <functional>
#include <format>

namespace RexGlueSuite {
    struct ProcessInfo {
      std::filesystem::path _basePath; 
      std::function<void()> on_success; 
      std::function<void(std::string err)> on_error;
      std::function<void(std::string log)> on_log;
    };

    class ExtractionRules {
      public:
        //Prevent unauthorized directory travel
        static bool IsPathSafe(const std::filesystem::path& fullPath, std::optional<uint64_t> sector = std::nullopt, const std::string& msgCaller = "rexglue_suite_extractionrules"){
          auto name = fullPath.string();
          if (name.find("..") != std::string::npos ||
            name.find("\\..") != std::string::npos ||
            name.find("../") != std::string::npos) {
            if( sector.has_value() ){
                REXLOG_ERROR("[{}] Unsafe directory name detected at sector {}", msgCaller, *sector);
            }else{
                REXLOG_ERROR("[{}] Unsafe directory name detected", msgCaller);
            }
            return false;
          }
          return true;
        }

        // Calculate available disk space and required available disk space
        // Adds a buffer that's 5% of the total extraction size, or 512mb at the least.
        static bool HasRequiredSpace( std::filesystem::path _basePath, uint64_t totalBytes, std::function<void(std::string err)> on_error, std::string msgCaller ="rexglue_suite_extractionrules"){
          std::error_code ec;
          auto space = std::filesystem::space(_basePath, ec);
          uint64_t requiredSpace = totalBytes + std::max<uint64_t>(512ull * 1024 * 1024, static_cast<uint64_t>(totalBytes * 0.05));
          if (!ec && space.available < requiredSpace ) {
            on_error(std::format("[{}] Not enough disk space: {} bytes is required, have {} available", msgCaller, requiredSpace, space.available));
            return false;
          }
          return true;
        }
    };
}