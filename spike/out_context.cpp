/*  Spike is universal dedicated module handler
    This souce contains data output context
    Part of PreCore project

    Copyright 2021-2022 Lukas Cone

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "out_context.hpp"
#include "console.hpp"
#include "datas/binreader.hpp"
#include "datas/crc32.hpp"
#include "datas/fileinfo.hpp"
#include "datas/stat.hpp"
#include "formats/ZIP_istream.inl"
#include "formats/ZIP_ostream.inl"
#include <chrono>
#include <mutex>

void ZIPExtactContext::FinishZIP(cache_begin_cb cacheBeginCB) {
  FinishFile(true);

  auto entriesStr = entriesStream.str();
  es::Dispose(entriesStream);
  bool forcex64 = false;
  const size_t dirOffset = records.Tell();
  size_t dirSize = entriesStr.size();

  auto SafeCast = [&](auto &where, auto &&what) {
    const uint64 limit =
        std::numeric_limits<std::decay_t<decltype(where)>>::max();
    if (what >= limit) {
      forcex64 = true;
      where = limit;
    } else {
      where = what;
    }
  };

  ZIPCentralDir zCentral{};
  zCentral.id = ZIPCentralDir::ID;
  SafeCast(zCentral.numDirEntries, numEntries);
  SafeCast(zCentral.numDiskEntries, numEntries);
  SafeCast(zCentral.dirOffset, dirOffset);

  records.WriteContainer(entriesStr);
  es::Dispose(entriesStr);

  if (cache) {
    records.Write<uint16>(0x4353);
    records.Write<uint16>(sizeof(CacheBaseHeader));
    cache->meta.zipCheckupOffset = records.Tell();
    records.Write(cache->meta);
    dirSize += sizeof(CacheBaseHeader) + 4;
  }

  SafeCast(zCentral.dirSize, dirSize);

  if (forcex64) {
    ZIP64CentralDir zCentral64{};
    zCentral64.id = ZIP64CentralDir::ID;
    zCentral64.madeBy = 10;
    zCentral64.extractVersion = 10;
    zCentral64.dirRecord = 44;
    zCentral64.numDiskEntries = numEntries;
    zCentral64.numDirEntries = numEntries;
    zCentral64.dirSize = dirSize;
    zCentral64.dirOffset = dirOffset;

    const size_t centralOffset = records.Tell();
    records.Write(zCentral64);

    ZIP64CentralDirLocator zLoca{};
    zLoca.id = ZIP64CentralDirLocator::ID;
    zLoca.centralDirOffset = centralOffset;
    records.Write(zLoca);
  }

  records.Write(zCentral);

  if (cache) {
    cacheBeginCB();
    cache->meta.zipSize = records.Tell();
    BinWritter cacheWr(outputFile + ".cache");
    cache->Write(cacheWr);
    records.Seek(cache->meta.zipCheckupOffset);
    records.Write(cache->meta);
  }
}

void ZIPExtactContext::FinishFile(bool final) {
  auto SafeCast = [&](auto &where, auto &&what) {
    const uint64 limit =
        std::numeric_limits<std::decay_t<decltype(where)>>::max();
    bool forcex64 = false;

    if (what >= limit) {
      forcex64 = true;
      where = limit;
    } else {
      where = what;
    }

    return forcex64;
  };

  const bool useLocalExtendedData =
      SafeCast(zLocalFile.uncompressedSize, curFileSize);
  zLocalFile.compressedSize = zLocalFile.uncompressedSize;

  if (useLocalExtendedData) {
    ZIP64Extra extra;
    extra.compressedSize = curFileSize;
    extra.uncompressedSize = curFileSize;
    records.Write(extra);
    zLocalFile.extraFieldSize = 20;
  }

  numEntries++;
  records.Push();
  records.Seek(curLocalFileOffset);
  records.Write(zLocalFile);
  const size_t fileDataBegin =
      records.Tell() + zLocalFile.extraFieldSize + zLocalFile.fileNameSize;

  if (cache) {
    cache->AddFile(curFileName, fileDataBegin, curFileSize);
    cache->meta.zipCRC = crc32b(
        cache->meta.zipCRC, reinterpret_cast<const char *>(&zLocalFile.crc), 4);
  } else {
    fileOffsets.push_back(fileDataBegin);
  }

  records.Pop();

  ZIPFile zFile{};
  zFile.id = ZIPFile::ID;
  zFile.madeBy = 10;
  zFile.extractVersion = 10;
  zFile.lastModFileDate = zLocalFile.lastModFileDate;
  zFile.lastModFileTime = zLocalFile.lastModFileTime;
  zFile.compression = ZIPCompressionMethod::Store;
  zFile.compressedSize = zLocalFile.compressedSize;
  zFile.uncompressedSize = zLocalFile.uncompressedSize;
  zFile.fileNameSize = zLocalFile.fileNameSize;
  zFile.crc = zLocalFile.crc;
  const bool useFileExtendedData =
      SafeCast(zFile.localHeaderOffset, curLocalFileOffset);

  const bool useFileExtra = useFileExtendedData || useLocalExtendedData;
  ZIP64Extra extra;

  if (useFileExtra) {
    zFile.extraFieldSize = 4;

    if (useLocalExtendedData) {
      extra.uncompressedSize = curFileSize;
      extra.compressedSize = curFileSize;
      zFile.extraFieldSize += 16;
    }

    if (useFileExtendedData) {
      extra.localHeaderOffset = curLocalFileOffset;
      zFile.extraFieldSize += 8;
    }
  }

  if (final && cache) {
    zFile.extraFieldSize += sizeof(CacheBaseHeader) + 4;
  }

  entries.Write(zFile);
  entries.WriteContainer(prefixPath);
  entries.WriteContainer(curFileName);

  if (useFileExtra) {
    entries.Write(extra);
  }
}

inline std::tm localtime(std::time_t t) {
#ifdef _MSC_VER
  return *std::localtime(&t);
#else
  std::tm temp;
  localtime_r(&t, &temp);
  return temp;
#endif
}

void ZIPExtactContext::NewFile(const std::string &path) {
  AFileInfo pathInfo(path);
  auto pathSv = pathInfo.GetFullPath();
  if (!curFileName.empty()) {
    FinishFile();
  }

  time_t curTime =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm ts = localtime(curTime);

  struct {
    uint16 day : 5, month : 4, year : 7;
  } dosDate{uint16(ts.tm_mday), uint16(ts.tm_mon + 1), uint16(ts.tm_year - 80)};

  struct {
    uint16 second : 5, minute : 6, hour : 5;
  } dosTime{uint16(ts.tm_sec / 2), uint16(ts.tm_min), uint16(ts.tm_hour)};

  zLocalFile.lastModFileDate = reinterpret_cast<uint16 &>(dosDate);
  zLocalFile.lastModFileTime = reinterpret_cast<uint16 &>(dosTime);
  zLocalFile.compression = ZIPCompressionMethod::Store;
  zLocalFile.fileNameSize = prefixPath.size() + pathSv.size();
  zLocalFile.crc = 0;
  curFileSize = 0;

  curFileName = pathSv;
  curLocalFileOffset = records.Tell();
  records.Write(zLocalFile);
  records.WriteContainer(prefixPath);
  records.WriteContainer(pathSv);

  if (progBar) {
    (*progBar)++;
  }

  if (totalBar) {
    (*totalBar)++;
  }
}

void ZIPExtactContext::SendData(es::string_view data) {
  curFileSize += data.size();
  zLocalFile.crc = crc32b(zLocalFile.crc, data.data(), data.size());
  records.WriteContainer(data);
}

bool ZIPExtactContext::RequiresFolders() const { return false; }

void ZIPExtactContext::AddFolderPath(const std::string &) {
  throw std::logic_error(
      "AddFolderPath not supported, use RequiresFolders to check.");
}

void ZIPExtactContext::GenerateFolders() {
  throw std::logic_error(
      "GenerateFolders not supported, use RequiresFolders to check.");
}

void IOExtractContext::NewFile(const std::string &path) {
  Close_();
  AFileInfo cfleWrap(path);
  auto cfle = cfleWrap.GetFullPath();
  Open(outDir + cfle.to_string());

  if (progBar) {
    (*progBar)++;
  }
  if (totalBar) {
    (*totalBar)++;
  }
}

void IOExtractContext::SendData(es::string_view data) { WriteContainer(data); }

bool IOExtractContext::RequiresFolders() const { return true; }

void IOExtractContext::AddFolderPath(const std::string &path) {
  AFileInfo cfleWrap(path);
  auto cfle = cfleWrap.GetFullPath();

  for (auto it = cfle.begin(); it != cfle.end(); it++) {
    if (*it == '/') {
      folderTree.emplace(cfle.begin(), it);
    }
  }

  folderTree.emplace(path);
}

void IOExtractContext::GenerateFolders() {
  for (auto &f : folderTree) {
    auto genFolder = outDir + f;
    es::mkdir(genFolder);
  }

  folderTree.clear();
}

static std::mutex ZIPLock;

void ZIPMerger::Merge(ZIPExtactContext &other, const std::string &recordsFile) {
  if (!other.curFileName.empty()) {
    other.FinishFile();
  }

  BinReaderRef localEntries(other.entriesStream);
  char buffer[0x80000];
  std::lock_guard<std::mutex> guard(ZIPLock);
  const size_t filesSize = records.Tell();

  numEntries += other.numEntries;

  for (auto o : other.fileOffsets) {
    ZIPFile zFile;
    localEntries.Read(zFile);

    const size_t localHeaderOffset = zFile.localHeaderOffset;
    const bool newExtraField = zFile.localHeaderOffset < 0xffffffff &&
                               localHeaderOffset + filesSize >= 0xffffffff;
    bool newExtra = false;

    if (newExtraField) {
      zFile.localHeaderOffset = 0xffffffff;

      if (!zFile.extraFieldSize) {
        zFile.extraFieldSize = 12;
        newExtra = true;
      } else {
        zFile.extraFieldSize += 8;
      }
    }

    if (zFile.localHeaderOffset < 0xffffffff) {
      zFile.localHeaderOffset += filesSize;
    }

    entries.Write(zFile);
    localEntries.ReadBuffer(buffer, zFile.fileNameSize);
    cache.AddFile({buffer, zFile.fileNameSize}, o + filesSize,
                  zFile.uncompressedSize);
    cache.meta.zipCRC = crc32b(cache.meta.zipCRC,
                               reinterpret_cast<const char *>(&zFile.crc), 4);
    entries.WriteBuffer(buffer, zFile.fileNameSize);

    if (newExtra) {
      ZIP64Extra extra{};
      extra.localHeaderOffset = localHeaderOffset + filesSize;
      entries.Write(extra);
    } else if (zFile.extraFieldSize) {
      ZIP64Extra extra{};
      localEntries.Read(extra.id);
      localEntries.Read(extra.size);

      if (zFile.compressedSize == 0xffffffff) {
        localEntries.Read(extra.compressedSize);
        localEntries.Read(extra.uncompressedSize);
      }

      if (zFile.localHeaderOffset == 0xffffffff) {
        if (newExtraField) {
          extra.localHeaderOffset = localHeaderOffset + filesSize;
        } else {
          localEntries.Read(extra.localHeaderOffset);
          extra.localHeaderOffset += filesSize;
        }
      }

      entries.Write(extra);
    }
  }

  es::Dispose(other.entriesStream);

  const size_t recordsSize = other.records.Tell();
  es::Dispose(other.records);
  BinReader rd(recordsFile);
  const size_t numBlocks = recordsSize / sizeof(buffer);
  const size_t restBytes = recordsSize % sizeof(buffer);

  for (size_t b = 0; b < numBlocks; b++) {
    rd.ReadBuffer(buffer, sizeof(buffer));
    records.WriteBuffer(buffer, sizeof(buffer));
  }

  if (restBytes) {
    rd.ReadBuffer(buffer, restBytes);
    records.WriteBuffer(buffer, restBytes);
  }
}

void ZIPMerger::FinishMerge(cache_begin_cb cacheBeginCB) {
  const size_t entriesSize = entries.Tell();
  es::Dispose(entries);
  char buffer[0x80000];
  BinReader rd(entriesFile);
  const size_t numBlocks = entriesSize / sizeof(buffer);
  const size_t restBytes = entriesSize % sizeof(buffer);
  bool forcex64 = true;
  const size_t dirOffset = records.Tell();

  auto SafeCast = [&](auto &where, auto &&what) {
    const uint64 limit =
        std::numeric_limits<std::decay_t<decltype(where)>>::max();
    if (what >= limit) {
      forcex64 = true;
      where = limit;
    } else {
      where = what;
    }
  };

  ZIPCentralDir zCentral{};
  zCentral.id = ZIPCentralDir::ID;
  SafeCast(zCentral.numDirEntries, numEntries);
  SafeCast(zCentral.numDiskEntries, numEntries);
  SafeCast(zCentral.dirSize, entriesSize);
  SafeCast(zCentral.dirOffset, dirOffset);

  for (size_t b = 0; b < numBlocks; b++) {
    rd.ReadBuffer(buffer, sizeof(buffer));
    records.WriteBuffer(buffer, sizeof(buffer));
  }

  if (restBytes) {
    rd.ReadBuffer(buffer, restBytes);
    records.WriteBuffer(buffer, restBytes);
  }

  bool validCacheEntry = false;

  {
    // Find last zipfile entry and modify extraFieldSize
    const size_t skipValue = std::min(entriesSize, size_t(0x11000));
    rd.Skip(-skipValue);
    rd.ReadBuffer(buffer, skipValue);
    std::string_view sv(buffer, skipValue);
    size_t foundLastEntry = sv.find_last_of("PK\x01\x02");
    validCacheEntry = foundLastEntry != sv.npos;

    if (validCacheEntry) {
      foundLastEntry += offsetof(ZIPFile, extraFieldSize);
      uint16 extraFieldSize =
          *reinterpret_cast<uint16 *>(buffer + foundLastEntry);
      records.Push();
      records.Skip(-(skipValue - foundLastEntry + 3));
      records.Write<uint16>(extraFieldSize + 4 + sizeof(CacheBaseHeader));
      records.Pop();

      records.Write<uint16>(0x4353);
      records.Write<uint16>(sizeof(CacheBaseHeader));
      cache.meta.zipCheckupOffset = records.Tell();
      records.Write(cache.meta);
    }
  }

  if (forcex64) {
    ZIP64CentralDir zCentral64{};
    zCentral64.id = ZIP64CentralDir::ID;
    zCentral64.madeBy = 10;
    zCentral64.extractVersion = 10;
    zCentral64.numDiskEntries = numEntries;
    zCentral64.numDirEntries = numEntries;
    zCentral64.dirSize = entriesSize;
    zCentral64.dirOffset = dirOffset;

    const size_t centralOffset = records.Tell();
    records.Write(zCentral64);

    ZIP64CentralDirLocator zLoca{};
    zLoca.id = ZIP64CentralDirLocator::ID;
    zLoca.centralDirOffset = centralOffset;
    records.Write(zLoca);
  }

  records.Write(zCentral);

  es::Dispose(rd);
  es::RemoveFile(entriesFile);

  if (validCacheEntry) {
    cacheBeginCB();
    cache.meta.zipSize = records.Tell();
    BinWritter cacheWr(outFile + ".cache");
    cache.Write(cacheWr);
    records.Seek(cache.meta.zipCheckupOffset);
    records.Write(cache.meta);
  }
}
