// condemned2recomp - ReXGlue Recompiled Project
//
// File downloader using curl 

#pragma once

#include "picosha2.h"
#include <curl/curl.h>
#include <fstream>
#include <atomic>
#include <thread>

namespace RexGlueSuite {
    class Downloader {
        public:
            static std::string sha256(std::string_view data) {
                std::string hash256_hex;
                picosha2::hash256_hex_string(data.begin(), data.end(), hash256_hex);
                return hash256_hex;
            }

            static constexpr size_t kMaxDownloadSize = 256ull * 1024 * 1024; 
            static size_t write_to_vector(void* ptr, size_t size, size_t nmemb, void* userdata) {
                auto* buffer = static_cast<std::vector<uint8_t>*>(userdata);
                size_t total = size * nmemb;
                if (buffer->size() + total > kMaxDownloadSize) {
                    return 0;
                }
                uint8_t* data = static_cast<uint8_t*>(ptr);
                buffer->insert(buffer->end(), data, data + total);
                return total;
            }

            static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
                auto* atomics = static_cast<std::pair<std::atomic<uint64_t>*, std::atomic<uint64_t>*>*>(clientp);
                if (dltotal > 0) {
                    *(atomics->second) = static_cast<uint64_t>(dltotal); 
                }
                *(atomics->first) = static_cast<uint64_t>(dlnow);
                return 0;
            }

            static bool Download(std::string url, std::string hash, ProcessInfo _processInfo, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize)
            {
                CURL* curl = curl_easy_init();
                if (!curl){
                    _processInfo.on_error("[rexglue_suite_downloader] Failed to init curl");
                    return false;
                }

                std::vector<uint8_t> bytes;
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_vector);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);

                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

                curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
                
                std::pair<std::atomic<uint64_t>*, std::atomic<uint64_t>*> progressData{&_extractedBytes, &_fullSize};
                curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
                curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progressData);

                CURLcode res = curl_easy_perform(curl);
                long httpCode = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
                curl_easy_cleanup(curl);

                if (httpCode != 200) {
                    _processInfo.on_error(std::format("[rexglue_suite_downloader] Server returned HTTP {}", httpCode));
                    return false;
                }

                if (res != CURLE_OK){
                    _processInfo.on_error(std::format("[rexglue_suite_downloader] Curl error {}: {} (HTTP {})", (int)res, curl_easy_strerror(res), httpCode));
                    return false;
                }

                const std::string result = sha256(std::string_view(reinterpret_cast<const char*>(bytes.data()),bytes.size()));
                if (result != hash) {
                    _processInfo.on_error(std::format("[rexglue_suite_downloader] SHA-256 mismatch. Target={}, Result={}", hash, result));
                    return false;
                }
                REXLOG_INFO("[rexglue_suite_downloader] SHA-256 match. Target={}, Result={}", hash, result);

                std::ofstream outFile(_processInfo._basePath, std::ios::binary | std::ios::trunc);
                if (!outFile) {
                    _processInfo.on_error(std::format("[rexglue_suite_downloader] Failed to create {}", _processInfo._basePath.string()));
                    return false;
                }

                outFile.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                if (!outFile) {
                    _processInfo.on_error(std::format("[rexglue_suite_downloader] Failed writing downloaded data to {}", _processInfo._basePath.string()));
                    return false;
                }

                REXLOG_INFO("[rexglue_suite_downloader] Downloaded {} bytes -> {}", bytes.size(), _processInfo._basePath.string());
                _processInfo.on_success();
                return true;
            }

            static std::thread NewThread(std::string url, std::string hash, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo){
                return std::thread (Download, url, hash, _processInfo, std::ref(_extractedBytes), std::ref(_fullSize)); 
            }
    };
}