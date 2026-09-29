// condemned2recomp - ReXGlue Recompiled Project
//
// Xbox 360 XDVDFS ISO Extractor
//
// Applies the same safety principles as ISO9660/UDF parsers.
//
// Sources:
// https://xboxdevwiki.net/XDVDFS
// https://multimedia.cx/xdvdfs.html

#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <format>
#include <fstream>
#include <set>
#include <unordered_set>
#include <cstring>
#include <atomic>
#include <thread>
#include <suite/extraction_rules.h>

namespace RexGlueSuite {
    #pragma pack(push, 1)
    struct XdvdfsVolumeDescriptor {
      char     magicTop[20]; 
      uint32_t rootDirTableSector;
      uint32_t rootDirTableSize;
      uint64_t timestamp;
      uint8_t  _padding[0x7C8]; 
      char     magicBottom[20];
    };

    enum class XdvdfsAttribute : uint8_t {
      READONLY = 0x01,
      HIDDEN = 0x02,
      SYSTEM = 0x04,
      DIRECTORY = 0x10,
      ARCHIVE = 0x20,
      NORMAL = 0x80,
    };
    inline bool hasAttribute(XdvdfsAttribute value, XdvdfsAttribute flag) {
      return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
    }

    struct XdvdfsDirEntry {
      uint16_t leftOffset;
      uint16_t rightOffset;
      uint32_t sector;
      uint32_t size;
      XdvdfsAttribute attributes; 
      uint8_t  nameLength;
    };
    #pragma pack(pop)

    struct XdvdfsFileEntry{
      std::filesystem::path fullPath;
      uint64_t sector; 
      uint32_t size;
    };

    class Xdvdfs {
      public:
        Xdvdfs() {}

        static constexpr const uint64_t KNOWN_PARTITION_BASE_SECTORS[] = { 0ull, 0xFD90000ull, 0x2080000ull };
        static constexpr const char* MAGIC = "MICROSOFT*XBOX*MEDIA";
        static const size_t MAGIC_LEN = 20;
        static const size_t SECTOR_SIZE = 2048;
        static const size_t SCAN_SECTOR_SIZE = 250000;
        static const size_t BUFFER_SIZE = 4 * 1024 * 1024;
        static constexpr size_t MAX_OFFSETS = 500000; 
        static constexpr size_t MAX_DIRECTORIES = 50000;

        static uint64_t IsMagicNumber(uint64_t sector, char *sectorBuf, std::ifstream& f){
          f.seekg(sector * SECTOR_SIZE);
          f.read(sectorBuf, SECTOR_SIZE);
          if (f.gcount() < MAGIC_LEN){
            return 0;
          }
          if (std::memcmp(sectorBuf, MAGIC, MAGIC_LEN) == 0) {
            return sector;
          }
          return 0;
        }

        static uint64_t FindVolumeDescriptor(std::ifstream& f){   
          char sectorBuf[SECTOR_SIZE];
          uint64_t volume_desc;
          for (uint64_t base : KNOWN_PARTITION_BASE_SECTORS) {
            f.clear();
            uint64_t newBase = ( base / SECTOR_SIZE ) + 32;
            volume_desc = IsMagicNumber(newBase, sectorBuf, f); 
            if (volume_desc != 0) { 
              REXLOG_INFO("[rexglue_suite_xdvdfs] Known volume descriptor at sector {}", newBase);
              return volume_desc; 
            }
          }

          //In case we're handling an abnormal disk dump
          f.clear();
          f.seekg(0, std::ios::beg);
          std::vector<char> chunk(BUFFER_SIZE);
          uint64_t sectorBase = 0;
          const uint64_t maxSector = SCAN_SECTOR_SIZE;
          while(sectorBase < maxSector){
            f.read(chunk.data(), chunk.size() );
            std::streamsize got = f.gcount();
            if (got < static_cast<std::streamsize>(MAGIC_LEN)) break;
            for( std::streamsize off = 0; off + static_cast<std::streamsize>(MAGIC_LEN) <= got; off += SECTOR_SIZE){
              if (std::memcmp(&chunk[off], MAGIC, MAGIC_LEN) == 0) {
                uint64_t foundSector = sectorBase + ( off / SECTOR_SIZE );
                REXLOG_INFO("[rexglue_suite_xdvdfs] Found volume descriptor at sector {}", foundSector);
                return foundSector;
              }
            }
            sectorBase += got / SECTOR_SIZE;
            if (got < static_cast<std::streamsize>(chunk.size())) break;
          }
          return 0;
        }

        static bool IsInsideValidRange(uint64_t partitionBaseSector, uint64_t isoFileSize, uint64_t sector, uint32_t size, bool isFile = false){
          if (!isFile && size == 0){
            REXLOG_ERROR("[rexglue_suite_xdvdfs] Directory size is 0.");
            return false;
          }
          uint64_t entryStart = (partitionBaseSector + sector) * SECTOR_SIZE;
          uint64_t entryEnd = entryStart + size;
          if (entryEnd > isoFileSize || entryEnd < entryStart ) {
            REXLOG_ERROR("[rexglue_suite_xdvdfs] Entry out of ISO bounds, sector {} size {}", sector, size);
            return false; 
          }
          return true;
        }

        static void WalkEntries(std::vector<XdvdfsFileEntry>& requests, std::vector<XdvdfsFileEntry>& dirs, std::vector<uint16_t>& offsets, 
          std::filesystem::path _basePath, std::vector<char>& dirBuf, uint16_t entryOffset, uint64_t partitionBaseSector, uint64_t isoFileSize, const std::string& path, 
                    std::set<uint16_t>& openedEntries, std::vector<std::pair<uint64_t, uint64_t>>& claimedExtents) {
          if (entryOffset == 0xFFFF) return;

          if (!openedEntries.insert(entryOffset).second) {
            if(entryOffset > 0) {
              REXLOG_INFO("[rexglue_suite_xdvdfs] cycle detected at offset {} — stopping this branch", entryOffset);
            }
            return;
          }

          size_t byteOffset = static_cast<size_t>(entryOffset) * 4;
          if (byteOffset + sizeof(XdvdfsDirEntry) > dirBuf.size()) {
            REXLOG_INFO("[rexglue_suite_xdvdfs] unsafe directory traversal at offset {} — stopping this branch", entryOffset);
            return;
          }

          XdvdfsDirEntry entryStorage;
          std::memcpy(&entryStorage, &dirBuf[byteOffset], sizeof(XdvdfsDirEntry));
          XdvdfsDirEntry* entry = &entryStorage;
          auto pushOffset = [&](uint16_t off){
            if (off != 0xFFFF && (size_t(off) * 4) < dirBuf.size()) {
              offsets.push_back(off);
            }
          };
          pushOffset(entry->leftOffset);
          pushOffset(entry->rightOffset);

          if (entry->nameLength == 0) {
            REXLOG_INFO("[rexglue_suite_xdvdfs] Entry name length is 0");
            return;
          }

          size_t nameStart = byteOffset + sizeof(XdvdfsDirEntry);
          size_t nameEnd = nameStart + entry->nameLength;
          if (nameEnd > dirBuf.size()){
            REXLOG_INFO("[rexglue_suite_xdvdfs] Name out of bounds at offset {}", entryOffset);
            return;
          }

          bool isDir = hasAttribute(entry->attributes, XdvdfsAttribute::DIRECTORY );
          if(!IsInsideValidRange(partitionBaseSector, isoFileSize, entry->sector, entry->size, !isDir)){
            return;
          }

          uint64_t absSector = partitionBaseSector + entry->sector;
          uint64_t absEnd = absSector + (entry->size + SECTOR_SIZE - 1) / SECTOR_SIZE;
          for (auto& [usedStart, usedEnd] : claimedExtents) {
            if (!(absEnd <= usedStart || absSector >= usedEnd)) {
              REXLOG_ERROR("[rexglue_suite_xdvdfs] Overlapping extent detected at sector {}", entry->sector);
              return;
            }
          }
          claimedExtents.emplace_back(absSector, absEnd);
          
          std::string name(&dirBuf[nameStart], entry->nameLength);
          std::string fullPath = path.empty() ? name : path + "/" + name;
          if (isDir && !name.empty() && name.front() == '$') {
            REXLOG_INFO("[rexglue_suite_xdvdfs] Skipping system directory: {}", fullPath);
            return;
          }

          std::filesystem::path outPath = _basePath / fullPath;
          XdvdfsFileEntry fileEntry = { outPath.string(), partitionBaseSector + entry->sector, entry->size};
          if(!ExtractionRules::IsPathSafe(fileEntry.fullPath, fileEntry.sector, "rexglue_suite_xdvdfs")){
            return;
          }

          REXLOG_INFO("[rexglue_suite_xdvdfs] entry: {} | sector: {} | size: {} | dir: {}", fullPath, entry->sector, entry->size, isDir);

          if (isDir) {
            dirs.push_back({ fullPath, entry->sector, entry->size });
          } else {
            requests.push_back(fileEntry);
            REXLOG_INFO("[rexglue_suite_xdvdfs] Added file to requested extractions: {} ({} bytes)", outPath.string(), entry->size);
          }
        }

        static bool ParseDirectory(std::vector<XdvdfsFileEntry>& requests, std::vector<XdvdfsFileEntry>& dirs, std::filesystem::path _basePath, 
                std::ifstream& f, uint64_t partitionBaseSector, uint64_t isoFileSize, const std::string& path, std::function<void(std::string err)> on_error) {
          std::unordered_set<uint32_t> openedDirs;
          std::vector<std::pair<uint64_t, uint64_t>> claimedExtents;
          size_t dirCount = 0;
          size_t offsetCount = 0;
          while(!dirs.empty()){
            XdvdfsFileEntry entry = dirs.back();
            dirs.pop_back();
            if (++dirCount > MAX_DIRECTORIES) {
              on_error(std::format("[rexglue_suite_xdvdfs] Too many directories encountered ({}) — aborting", dirCount));
              return false;
            }

            if (!openedDirs.insert(static_cast<uint32_t>(entry.sector)).second) {
              REXLOG_INFO("[rexglue_suite_xdvdfs] Directory cycle detected at sector {} — skipping", entry.sector);
              continue;
            }

            if (!ExtractionRules::IsPathSafe(entry.fullPath, entry.sector, "rexglue_suite_xdvdfs")){
              continue;
            }

            if(!IsInsideValidRange(partitionBaseSector, isoFileSize, entry.sector, entry.size)){
              continue;
            }

            f.seekg((partitionBaseSector + entry.sector) * SECTOR_SIZE);
            std::vector<char> dirBuf(entry.size);
            f.read(dirBuf.data(), entry.size);
            if (f.gcount() != entry.size){
              REXLOG_INFO("[rexglue_suite_xdvdfs] Could not parse full directory entry {}", entry.sector);
              continue;
            }

            //Walk through all entries discovered.
            std::set<uint16_t> openedEntries;
            std::vector<uint16_t> offsets;
            offsets.push_back(0);
            while(!offsets.empty()){
              uint16_t off = offsets.back();
              offsets.pop_back();
              if (++offsetCount > MAX_OFFSETS) {
                on_error(std::format("[rexglue_suite_xdvdfs] Too many entries encountered ({}) — aborting", offsetCount));
                return false;
              }
              WalkEntries(requests, dirs, offsets, _basePath, dirBuf, off, partitionBaseSector, isoFileSize, entry.fullPath.string(), openedEntries, claimedExtents);
            }
          }
          return true;
        }

        static void ExtractFile(XdvdfsFileEntry request, std::ifstream& f, std::atomic<uint64_t>& _extractedBytes, const std::function<void(std::string log)>& on_log) {
          f.seekg(request.sector * SECTOR_SIZE);

          std::error_code ec;
          std::filesystem::create_directories(request.fullPath.parent_path(), ec);
          if (ec) {
            REXLOG_ERROR("[rexglue_suite_xdvdfs] Failed to create directory for {}: {}", request.fullPath.string(), ec.message());
            return;
          }

          std::ofstream outFile(request.fullPath, std::ios::binary | std::ios::trunc);
          if (!outFile) {
            REXLOG_ERROR("[rexglue_suite_xdvdfs] Failed to create {}", request.fullPath.string());
            return;
          }

          std::vector<char> buffer(BUFFER_SIZE);
          uint64_t remaining = request.size;
          while (remaining > 0) {
            const std::streamsize chunk = static_cast<std::streamsize>(std::min<uint64_t>(remaining, buffer.size()));
            f.read(buffer.data(), chunk);
            if (f.gcount() != chunk) {
              REXLOG_ERROR("[rexglue_suite_xdvdfs] Failed reading {} from ISO", request.fullPath.string());
              return;
            }

            outFile.write(buffer.data(), chunk);
            if (!outFile) {
              REXLOG_ERROR("[rexglue_suite_xdvdfs] Failed writing ISO to {}", request.fullPath.string());
              return;
            }
            remaining -= static_cast<uint64_t>(chunk);
            _extractedBytes += chunk;
          }
          on_log(std::format("[rexglue_suite_xdvdfs] Extracted: {} ({} bytes)", request.fullPath.string(), request.size));
        }

        static bool ExtractIso(std::filesystem::path _isoPath, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo ){
          _processInfo.on_log(std::format("[rexglue_suite_xdvdfs] Starting ISO Extraction: {}", _isoPath.string()));

          std::ifstream f( _isoPath, std::ios::binary);
          if (!f || !f.is_open()) {
            _processInfo.on_error("[rexglue_suite_xdvdfs] Failed to open selected ISO");
            return false;
          }

          uint64_t volDescSector = FindVolumeDescriptor(f);
          if (volDescSector == 0) {
            _processInfo.on_error("[rexglue_suite_xdvdfs] Magic string could not be found — check ISO format");
            return false;
          }

          f.seekg(volDescSector * SECTOR_SIZE);
          XdvdfsVolumeDescriptor volDesc;
          f.read(reinterpret_cast<char*>(&volDesc), sizeof(volDesc));
          if ( f.gcount() != sizeof(volDesc)){
            _processInfo.on_error("[rexglue_suite_xdvdfs] Failed to read full volume descriptor - unsupported or malformed ISO format");
            return false;
          }

          REXLOG_INFO("[rexglue_suite_xdvdfs] rootDirTableSector: {}, rootDirTableSize: {}", volDesc.rootDirTableSector, volDesc.rootDirTableSize);

          uint64_t isoFileSize = std::filesystem::file_size(_isoPath);
          uint64_t partitionBaseSector = volDescSector - 32;
          uint64_t rootDirAbsoluteSector = partitionBaseSector + volDesc.rootDirTableSector;

          f.seekg(rootDirAbsoluteSector * SECTOR_SIZE);

          std::vector<XdvdfsFileEntry> requests;
          std::vector<XdvdfsFileEntry> dirs;
          dirs.push_back({std::filesystem::path(""), volDesc.rootDirTableSector, volDesc.rootDirTableSize });
          if (!ParseDirectory(requests, dirs, _processInfo._basePath, f, partitionBaseSector, isoFileSize, "", _processInfo.on_error)) {
            return false;
          }

          //Tries to find "default.xex" in root directory
          bool hasDefaultXex = std::any_of(requests.begin(), requests.end(), [&](const XdvdfsFileEntry& e) {
            return e.fullPath.filename() == "default.xex" && e.fullPath.parent_path() == _processInfo._basePath;
          });
          if (!hasDefaultXex) {
            _processInfo.on_error("[rexglue_suite_xdvdfs] default.xex not found at disc root — not a valid Xbox 360 game disc.");
            return false;
          }

          uint64_t totalBytes = 0;
          for (const auto& request : requests) {
            totalBytes += request.size;
          }
          _fullSize = totalBytes;
          _processInfo.on_log(std::format("[rexglue_suite_xdvdfs] {} bytes to extract.", totalBytes));
          if( !ExtractionRules::HasRequiredSpace(_processInfo._basePath, totalBytes, _processInfo.on_error, "rexglue_suite_xdvdfs") ){
            return false;
          }

          //Sorts xex files after all other files, with "default.xex" extracted last to indicate a complete extraction.
          auto priority = [](const XdvdfsFileEntry& e) -> int {
            std::string filename = e.fullPath.filename().string();
            if (filename == "default.xex") return 2;
            if (filename.ends_with(".xex")) return 1;
            return 0;
          };
          std::sort(requests.begin(), requests.end(), [&](const XdvdfsFileEntry& a, const XdvdfsFileEntry& b) {
            int ra = priority(a), rb = priority(b);
            if (ra != rb) {
              return ra < rb;
            }
            return a.sector < b.sector;
          });

          //Walks through all file requests and extracts them
          for(const auto& request : requests){
            std::error_code ec;
            if (std::filesystem::is_regular_file(request.fullPath, ec) && std::filesystem::file_size(request.fullPath, ec) == request.size && !ec) {
              _extractedBytes += request.size;
              continue;
            }
            ExtractFile(request, f, _extractedBytes, _processInfo.on_log);
          }

          uint64_t bytesDone = _extractedBytes.load();
          REXLOG_INFO("[rexglue_suite_xdvdfs] ISO Extracted - {} -> {} ({} bytes)", _isoPath.string(), _processInfo._basePath.string(), bytesDone);

          _processInfo.on_success();
          return true;
        }

        static std::thread NewThread(std::filesystem::path _isoPath, std::atomic<uint64_t>& _extractedBytes, std::atomic<uint64_t>& _fullSize, ProcessInfo _processInfo ){
          return std::thread (ExtractIso, _isoPath, std::ref(_extractedBytes), std::ref(_fullSize), _processInfo ); 
        }
  };
}