// RexGlueSuite - Shared extraction safety utilities
//
// Retrieves Jsons, typically used for game updates!
#pragma once

#include <curl/curl.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <functional>
#include <format>

namespace RexGlueSuite {
    class CurlGetter {
      public:
        static std::string JsonStringOr(const nlohmann::json& object,const char* key,const char* fallback) {
            if (!object.is_object()) {
                return fallback;
            }
            const auto it = object.find(key);
            if (it == object.end() || !it->is_string()) {
                return fallback;
            }
            return it->get<std::string>();
        }

        static size_t write_to_string(void* ptr, size_t size, size_t nmemb, void* userdata)
        {
            auto* out = static_cast<std::string*>(userdata);
            out->append(static_cast<const char*>(ptr), size * nmemb);
            return size * nmemb;
        }

        static std::string GetJson(const std::string& url)
        {
            CURL* curl = curl_easy_init();
            if (!curl) throw std::runtime_error("curl_easy_init failed");

            std::string body;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "User-Agent: condemned2recomp-updater");
            headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            CURLcode rc = curl_easy_perform(curl);
            long code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (rc != CURLE_OK) throw std::runtime_error(curl_easy_strerror(rc));
            if (code < 200 || code >= 300) throw std::runtime_error("HTTP " + std::to_string(code));
            return body; // You're getting CURLED, buddy!
        }

        //If we can connect to Github, download updates and files from it!
        static bool CanConnectToGithub()
        {
            CURL* curl = curl_easy_init();
            if (!curl)
                return false;

            curl_easy_setopt(curl, CURLOPT_URL, "https://www.github.com");
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
            CURLcode result = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            return result == CURLE_OK;
        }
    };
}