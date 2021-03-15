/**
 * Copyright (C) 2021  Paschalis Mpeis (paschalis.mpeis-AT-gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef ART_RUNTIME_MCR_FILEREADER_H_
#define ART_RUNTIME_MCR_FILEREADER_H_

#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace art {
namespace mcr {

enum class Status {
  NotFound,
  Empty,
  CantOpen,
  OK,
  Unknown,
};
std::ostream& operator<<(std::ostream& os, const Status& rhs);

class FileReader {
 public:
  void readLines();

  std::vector<std::string> GetData() {
    readLines();
    return getLines();
  }

  std::vector<std::string>::iterator begin() {
    return lines_.begin();
  }

  std::vector<std::string>::iterator end() {
    return lines_.end();
  }

  FileReader(std::string from, std::string filename, bool term_on_not_found = true,
             bool term_on_empty = true, bool term_on_cant_open = true)
      : from_(from),
        filename_(filename),
        status_(Status::Unknown),
        term_on_not_found_(term_on_not_found),
        term_on_empty_(term_on_empty),
        term_on_cant_open_(term_on_cant_open) {
  }

  bool exists();
  bool isEmpty();
  bool hasPermissions();

  std::string getFilename() const {
    return filename_;
  }

 private:
  std::vector<std::string> getLines() const;

  std::ifstream in_;
  std::string from_;
  std::string filename_;
  std::vector<std::string> lines_;
  Status status_;
  bool term_on_not_found_;
  bool term_on_empty_;
  bool term_on_cant_open_;
  struct stat st_;
};

}  // namespace mcr
}  // namespace art
#endif  // ART_RUNTIME_MCR_FILEREADER_H_
