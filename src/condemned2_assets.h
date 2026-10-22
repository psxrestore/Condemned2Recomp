// condemned2recomp - ReXGlue Recompiled Project
//
// Condemned 2 Asset Managers

#pragma once

#include <cctype>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include <condemned2_hooks.h>

namespace Condemned2{
   class Condemned2Assets  {
        struct AssetRequest {
            uint32_t type;      // +0x00
            uint32_t owner;     // +0x04
            uint32_t assetKey;  // +0x08
        };
    public:
        explicit Condemned2Assets(Condemned2::Condemned2Hook *hook);
        ~Condemned2Assets();

        Condemned2::Condemned2Hook *GetHook() { return _hook; }

        // Bundles
        void AddLoadedBundle( std::string res, uint32_t handle ){ loadedBundles[res] = handle; }
        bool IsBundleLoaded( std::string res){return loadedBundles.find(res) != loadedBundles.end();}
        void ClearLoadedBundles() { loadedBundles.clear(); }
        std::unordered_map<std::string, uint32_t>& GetLoadedBundles() { return loadedBundles; }

        void SetLoadingBundle( bool enable ) { isLoadingBundle = enable; }
        bool IsLoadingBundle() { return isLoadingBundle; }
        void SetCanLoadBundle( bool enable ) { canLoadBundle = enable; }
        bool CanLoadBundle() { return canLoadBundle; }

        void SetCurrentBundle(std::string bundle) { currentBundle = bundle; }
        std::string GetCurrentBundle() { return currentBundle; }

        bool LoadAdditionalBundle(PPCContext& ctx, uint8_t* base, const std::string& bundlePath );
        void LoadAdditionalBundles(PPCContext& ctx, uint8_t* base);

        bool IsAdditionalBundle(const char* filename){
            if (filename == nullptr)
                return false;

            while (*filename == '\\' || *filename == '/')
                ++filename;

            const char* extension = std::strrchr(filename, '.');
            if (extension == nullptr ||
                std::strcmp(extension, ".bndl") != 0) {
                return false;
            }

            if (std::strcmp(filename, "interface\\common.bndl") == 0 ||
                std::strcmp(filename, "Interface\\Extras.bndl") == 0 ||
                std::strcmp(filename, "Interface\\MPWorldLoad.bndl") == 0 ||
                std::strcmp(filename, "Interface\\Startup.bndl") == 0 ||
                std::strcmp(filename, "Interface\\Shell.bndl") == 0) {
                return false;
            }
            return true;
        }

        // Resources
        void AddLoadedResource( std::string res ){ loadedAssets[res] = currentBundle; }
        bool IsResourceLoaded( std::string res){return loadedAssets.find(res) != loadedAssets.end();}
        void ClearLoadedResources() { loadedAssets.clear(); }

        // Debugging
        bool IsDebugging() { return isDebugging; }
        bool IsDebuggingResources() { return isDebuggingResources; }

      private:
        Condemned2::Condemned2Hook *_hook;
        std::unordered_map<std::string, std::string> loadedAssets;
        std::unordered_map<std::string, uint32_t> loadedBundles;
        std::string currentBundle;
        bool isLoadingBundle = false;
        bool canLoadBundle = false;
        bool isDebugging = false;
        bool isDebuggingResources = false;
   };
}  // namespace Condemned2