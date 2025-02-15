/*  source for DirectoryScanner class

    Copyright 2019-2021 Lukas Cone

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

#include "directory_scanner.hpp"

#if defined(_MSC_VER) || defined(__MINGW64__)
#define USEWIN
#include "tchar.hpp"
#include <windows.h>
#elif defined(__GNUC__) || defined(__GNUG__)
#include <dirent.h>
#endif

#include <cstring>

bool PathFilter::IsFiltered(es::string_view fileName_) const {
  if (!filters.size()) {
    return true;
  }

  es::string_view fileName(fileName_);

  for (auto &f : filters) {
    es::string_view kvi(f);
    bool clampBegin = kvi.front() == '^';
    bool clampEnd = kvi.back() == '$';

    if (clampBegin) {
      kvi.remove_prefix(1);
    }

    if (clampEnd) {
      kvi.remove_suffix(1);
    }

    auto wildcharPos = kvi.find_first_of('*');
    bool useWildchar = wildcharPos != kvi.npos;

    if (useWildchar) {
      auto part1 = kvi.substr(0, wildcharPos);
      auto part2 = kvi.substr(wildcharPos + 1);

      // cases ^foo*bar or ^foo*bar$
      if (clampBegin) {
        if (fileName.begins_with(part1)) {
          if ((clampEnd && fileName.ends_with(part2)) || !clampEnd) {
            return true;
          }
        }
      }
      // cases foo*bar$ only
      else if (clampEnd) {
        if (fileName.ends_with(part2) &&
            fileName.find(part1) != fileName.npos) {
          return true;
        }
      }
      // cases foo*bar only
      else if (fileName.find(part1) != fileName.npos &&
               fileName.find(part2) != fileName.npos) {
        return true;
      }
    } else if (clampBegin) {
      if (fileName.begins_with(kvi)) {
        return true;
      }
    } else if (clampEnd) {
      if (fileName.ends_with(kvi)) {
        return true;
      }
    } else if (fileName.find(kvi) != f.npos) {
      return true;
    }
  }

  return false;
}

void DirectoryScanner::Scan(std::string dir) {
  if (!dir.empty()) {
    char lastWord = *std::prev(dir.end());

    if (lastWord == '"') {
      dir.pop_back();
    }

    if (lastWord != '\\' && lastWord != '/') {
      dir.push_back('/');
    }
  }

#ifndef USEWIN
  dir.push_back('.');

  DIR *cDir = opendir(dir.data());

  if (!cDir) {
    return;
  }

  dirent *cFile = nullptr;

  while ((cFile = readdir(cDir)) != nullptr) {
    if (!strcmp(cFile->d_name, ".") || !strcmp(cFile->d_name, "..")) {
      continue;
    }

    std::string miniFile(cFile->d_name);
    std::string subFile = dir;
    subFile.pop_back();
    subFile += miniFile;

    if (cFile->d_type == DT_DIR) {
      numFolders++;
      Scan(subFile);
    } else {
      numFiles++;
      if (IsFiltered(miniFile)) {
        files.push_back(subFile);
      }
    }

    if (scanCb) {
      scanCb(scanCbData, numFolders, numFiles, files.size());
    }
  }

  closedir(cDir);
#else
  dir.push_back('*');
  const auto wdir = ToTSTRING(dir);

  WIN32_FIND_DATA foundData = {};
  HANDLE fleHandle = FindFirstFile(wdir.data(), &foundData);

  if (fleHandle == INVALID_HANDLE_VALUE) {
    return;
  }

  while (FindNextFile(fleHandle, &foundData)) {
    if (!_tcscmp(foundData.cFileName, _T(".")) ||
        !_tcscmp(foundData.cFileName, _T("..")) ||
        (foundData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0) {
      continue;
    }

    std::string subFile = dir;
    subFile.pop_back();
    std::string cFileName = std::to_string(foundData.cFileName);
    subFile += cFileName;

    if ((foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      numFolders++;
      Scan(subFile);
    } else {
      numFiles++;
      if (IsFiltered(cFileName)) {
        files.push_back(subFile);
      }
    }

    if (scanCb) {
      scanCb(scanCbData, numFolders, numFiles, files.size());
    }
  }

  FindClose(fleHandle);
#endif
}

void DirectoryScanner::ScanFolders(std::string dir) {
  if (!dir.empty()) {
    char lastWord = *std::prev(dir.end());

    if (lastWord == '"') {
      dir.pop_back();
    }

    if (lastWord != '\\' && lastWord != '/') {
      dir.push_back('/');
    }
  }

#ifndef USEWIN
  dir.push_back('.');

  DIR *cDir = opendir(dir.data());

  if (!cDir) {
    return;
  }

  dirent *cFile = nullptr;

  while ((cFile = readdir(cDir)) != nullptr) {
    if (!strcmp(cFile->d_name, ".") || !strcmp(cFile->d_name, "..")) {
      continue;
    }

    std::string miniFile(cFile->d_name);
    std::string subFile = dir;
    subFile.pop_back();
    subFile += miniFile;

    if (cFile->d_type == DT_DIR) {
      files.push_back(subFile);
    }
  }

  closedir(cDir);
#else
  dir.push_back('*');
  const auto wdir = ToTSTRING(dir);

  WIN32_FIND_DATA foundData = {};
  HANDLE fleHandle = FindFirstFile(wdir.data(), &foundData);

  if (fleHandle == INVALID_HANDLE_VALUE) {
    return;
  }

  while (FindNextFile(fleHandle, &foundData)) {
    if (!_tcscmp(foundData.cFileName, _T(".")) ||
        !_tcscmp(foundData.cFileName, _T("..")) ||
        (foundData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) != 0) {
      continue;
    }

    std::string subFile = dir;
    subFile.pop_back();
    std::string cFileName = std::to_string(foundData.cFileName);
    subFile += cFileName;

    if ((foundData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      files.push_back(subFile);
    }
  }

  FindClose(fleHandle);
#endif
}
