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
    struct BundleAlias {
      std::string modelName;
      std::string bundlePath;
    };
    public:
        explicit Condemned2Assets(Condemned2::Condemned2Hook *hook);
        ~Condemned2Assets();

        Condemned2::Condemned2Hook *GetHook() { return _hook; }

        // Bundles
        // Bundles that the game has loaded in.
        void AddLoadedBundle( std::string res, uint32_t handle );
        bool IsBundleLoaded( std::string res){return loadedBundles.find(res) != loadedBundles.end();}
        std::unordered_map<std::string, uint32_t>& GetLoadedBundles() { return loadedBundles; }
        void ClearLoadedBundles() { loadedBundles.clear(); }

        // Bundles we've loaded in ourselves
        void AddOwnedBundle( std::string res, uint32_t handle );
        bool IsBundleOwned( std::string res){return ownedBundles.find(res) != ownedBundles.end();}
        std::unordered_map<std::string, uint32_t>& GetOwnedBundles() { return ownedBundles; }
        void ClearOwnedBundles() { ownedBundles.clear(); }

        // Allocating/Deallocating bundles
        bool LoadAdditionalBundle(PPCContext& ctx, uint8_t* base, const std::string& bundlePath );
        void ReleaseBundle(PPCContext& ctx,uint8_t* base,uint32_t handle);
        void UnloadBundles(PPCContext& ctx,uint8_t* base);

        // Identifying bundles
        bool IsAssetBundle(std::string filename);
        void SetCurrentBundle(std::string bundle) { currentBundle = bundle; }
        std::string GetCurrentBundle() { return currentBundle; }
        std::string GetBundlePathFromName(const std::string& name);
        std::string GetBundlePathFromModel(const std::string& name);
        std::string GetFilenameFromPath(const std::string& path);

        // Resources loaded from the bundles
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
        std::unordered_map<std::string, uint32_t> ownedBundles;
        std::string currentBundle;
        bool isDebugging = false;
        bool isDebuggingResources = false;
   };
}  // namespace Condemned2