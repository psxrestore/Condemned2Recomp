// condemned2recomp - ReXGlue Recompiled Project
//
// Title update extractor using Rexglue's STFS container

#pragma once

#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/system/xtypes.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_set>

#include <suite/extraction_rules.h>

using namespace rex::filesystem;

namespace RexGlueSuite {
    class TitleUpdater {
        public:
            struct TitleUpdateFileEntry{
                rex::filesystem::Entry *entry;
                std::filesystem::path fullPath;
            };

            static const size_t BUFFER_SIZE = 4 * 1024 * 1024;

            static bool ExtractEntry(TitleUpdateFileEntry request, std::atomic<uint64_t>& _extractedBytes, const std::function<void(std::string log)>& on_log) {
                rex::filesystem::File* file = nullptr;
                auto status = request.entry->Open(0, &file);
                if (status != 0x00000000L || !file) {
                    REXLOG_ERROR("[rexglue_suite_title_updater] Failed to open {}, status={}", request.entry->path(), status );
                    return false;
                }
         
                std::ofstream out(request.fullPath, std::ios::binary);
                if (!out) {
                    REXLOG_ERROR("[rexglue_suite_title_updater] Could not open output file: {}", request.fullPath.string() );
                    file->Destroy();
                    return false;
                }

                std::vector<uint8_t> buffer(BUFFER_SIZE);
                uint64_t total_size = request.entry->size();
                uint64_t offset = 0;
                uint64_t total_read = 0;
                while (offset < total_size) {
                    size_t chunk = static_cast<size_t>(std::min<uint64_t>(BUFFER_SIZE, total_size - offset));
                    size_t bytes_read = 0;
                    status = file->ReadSync(buffer, offset, &bytes_read);
                    if (bytes_read == 0 && chunk != 0) {
                        REXLOG_ERROR("[rexglue_suite_title_updater] Unexpected zero-byte read");
                        file->Destroy();
                        return false;
                    }

                    if (status != 0x00000000L) {
                        REXLOG_ERROR("[rexglue_suite_title_updater] ReadSync failed for {}, status={}", request.entry->path(), status );
                        file->Destroy();
                        return false;
                    }

                    out.write(reinterpret_cast<const char*>(buffer.data()), bytes_read);
                    if (!out) {
                        REXLOG_ERROR("[rexglue_suite_title_updater] Failed writing to {}", request.fullPath.string() );
                        file->Destroy();
                        return false;
                    }
                    _extractedBytes += bytes_read;
                    offset += bytes_read;
                    total_read += bytes_read;
                }
                
                out.flush();
                out.close();
                file->Destroy();
                on_log(std::format("[rexglue_suite_title_updater] Extracted {} ({} bytes)", request.fullPath.string(), total_read ));
                return true;
            }

            static bool ExtractAll(rex::filesystem::Entry* entry, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo ) {
                bool _success = true;
                uint64_t totalBytes = 0;
                std::error_code ec;

                std::vector<TitleUpdateFileEntry> dirs;
                std::vector<TitleUpdateFileEntry> requests;
                std::unordered_set<rex::filesystem::Entry*> openedEntries;
                dirs.push_back({entry, _processInfo._basePath});

                while(!dirs.empty()){
                    TitleUpdateFileEntry d = dirs.back();
                    dirs.pop_back();

                    if (!openedEntries.insert(d.entry).second) {
                        REXLOG_INFO("[rexglue_suite_title_updater] Cycle detected at path - {} — skipping", d.entry->path());
                        continue;
                    }

                    if (!ExtractionRules::IsPathSafe(d.fullPath, std::nullopt, "rexglue_suite_title_updater")) {
                        _success = false;
                        continue;
                    }

                    std::filesystem::create_directories(d.fullPath, ec);
                    if (ec) {
                        REXLOG_ERROR("[rexglue_suite_title_updater] Failed to create directory {}: {}", d.fullPath.string(), ec.message());
                        _success = false;
                        continue;
                    }

                    for (auto& child : d.entry->children()) {
                        rex::filesystem::Entry *childEntry = child.get();
                        if(childEntry){
                            std::filesystem::path child_out = d.fullPath / child->name();
                            if (!ExtractionRules::IsPathSafe(child_out, std::nullopt, "rexglue_suite_title_updater")) {
                                _success = false;
                                continue;
                            }

                            if (child->attributes() & kFileAttributeDirectory) {
                                dirs.push_back({childEntry, child_out});
                            } else {
                                requests.push_back({childEntry, child_out});
                                totalBytes += childEntry->size();
                            }
                        }
                    }
                }

                _fullSize = totalBytes;

                _processInfo.on_log(std::format("[rexglue_suite_title_updater] {} bytes to extract.", totalBytes));

                if( !ExtractionRules::HasRequiredSpace( _processInfo._basePath, totalBytes, _processInfo.on_error, "rexglue_suite_title_updater") ){
                    return false;
                }

                for (auto& request : requests) {
                    _success &= ExtractEntry(request, _extractedBytes, _processInfo.on_log);
                }
                return _success;
            }

            static bool Extract(const std::filesystem::path& stfsPath, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo){
                rex::filesystem::StfsContainerDevice device("tu:", stfsPath);
                if (!device.Initialize()) {
                    _processInfo.on_error(std::format("[rexglue_suite_title_updater] Failed to initialize STFS container: {}", stfsPath.string() ));
                    return false;
                }
                
                rex::filesystem::Entry* root = device.ResolvePath("");
                if (!root) {
                    _processInfo.on_error("[rexglue_suite_title_updater] Failed to resolve container root");
                    return false;
                }

                bool _success = ExtractAll(root, _extractedBytes, _fullSize, _processInfo);
                if(!_success){
                    _processInfo.on_error("[rexglue_suite_title_updater] Failed to extract one or more files!");
                    return false;
                }

                _processInfo.on_log("[rexglue_suite_title_updater] Extraction complete");
                _processInfo.on_success();
                return true;
            }

            static std::thread NewThread(std::filesystem::path _tuPath, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo){
                return std::thread (Extract, _tuPath, std::ref(_extractedBytes), std::ref(_fullSize),  _processInfo  ); 
            }
    };
}
