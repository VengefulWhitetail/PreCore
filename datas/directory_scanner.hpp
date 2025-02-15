/*  DirectoryScanner class is a simple class
    for scanning files inside a folder hierarchy

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

#pragma once
#include "datas/string_view.hpp"
#include "settings.hpp"
#include <string>
#include <vector>

class PathFilter {
public:
  bool PC_EXTERN IsFiltered(es::string_view name) const;

  /*
  Store only specified files with filename substring
  format: "[^]<substring>[*<substring>][$]"
  val begins with ^: clamp beginning
  val ends with $: clamp ending
  val contains *: wildchar handling
  otherwise: free substring search
  */
  void AddFilter(const std::string &val) {
    filterHolders.push_back(val);
    filters.push_back(filterHolders.back());
  }
  void AddFilter(es::string_view val) { filters.push_back(val); }
  void ClearFilters() { filters.clear(); }

private:
  std::vector<std::string> filterHolders;
  std::vector<es::string_view> filters;
};

class DirectoryScanner : public PathFilter {
public:
  typedef std::vector<std::string> storage_type;
  typedef storage_type::iterator iterator;
  typedef storage_type::const_iterator const_iterator;

  DirectoryScanner() = default;
  DirectoryScanner(const DirectoryScanner &) = default;
  DirectoryScanner(DirectoryScanner &&) = default;
  DirectoryScanner(const PathFilter &p) : PathFilter(p) {}
  DirectoryScanner(PathFilter &&p) : PathFilter(std::move(p)) {}

  DirectoryScanner &operator=(const DirectoryScanner &) = default;
  DirectoryScanner &operator=(DirectoryScanner &&) = default;

  void PC_EXTERN Scan(std::string dir);
  void PC_EXTERN ScanFolders(std::string dir);

  iterator begin() { return files.begin(); }
  iterator end() { return files.end(); }
  const_iterator cbegin() const { return files.cbegin(); }
  const_iterator cend() const { return files.cend(); }
  const storage_type &Files() const { return files; }
  void Clear() { files.clear(); }

  using scan_callback = void (*)(void *cbData, size_t numFolders,
                                 size_t numFiles, size_t foundFiles);
  scan_callback scanCb = nullptr;
  void *scanCbData = nullptr;

private:
  size_t numFiles = 0;
  size_t numFolders = 0;
  storage_type files;
};
