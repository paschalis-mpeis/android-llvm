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
#include "mcr_rt/filereader.h"

#include "mcr_rt/mcr_rt.h"
#include "runtime.h"

namespace art {
namespace mcr {

std::vector<std::string> FileReader::getLines() const {
  if (status_ != Status::OK) {
    DLOG(ERROR) << "mcr::FileReader(): (on behalf: " << from_
               << ") getLines(): Wrong status: " << status_
               << " From: " << from_
               << " File: " << filename_;
  }
  return lines_;
}

void FileReader::readLines() {
  if (!exists()) return;
  if (isEmpty()) return;

  in_.open(filename_.c_str());
  if (!hasPermissions()) return;

  std::string line;
  while (!in_.eof()) {
    std::getline(in_, line);
    if (in_.eof()) {
      break;
    }
    lines_.push_back(line);
  }
  in_.close();

  status_ = Status::OK;
}

bool FileReader::exists() {
  int err = stat(filename_.c_str(), &st_);
  if (err == -1) {
    status_ = Status::NotFound;
    if (term_on_not_found_) {
      DLOG(FATAL) << "mcr::FileReader(): (on behalf: " << from_
                 << ") not found: " << filename_ << " Error: " << strerror(errno);
    }
    return false;
  }
  return true;
}

bool FileReader::isEmpty() {
  if (st_.st_size == 0) {
    status_ = Status::Empty;
    if (term_on_empty_) {
      DLOG(FATAL) << "mcr::FileReader(): (on behalf: " << from_
                 << ") empty: " << filename_;
    }
    return true;
  }
  return false;
}

bool FileReader::hasPermissions() {
  if (!in_) {
    status_ = Status::CantOpen;

    DLOG(ERROR) << "mcr::FileReader(): (on behalf: " << from_
               << ") cant open: " << filename_;
    DLOG(VERBOSE) << "file owner: " << st_.st_uid << ":" << st_.st_gid;
    DLOG(VERBOSE) << "me: " << getuid() << ":" << getgid();
    DLOG(VERBOSE) << "file permissions: " << std::oct << st_.st_mode;
    DLOG(VERBOSE) << "errno: " << errno;

    if (term_on_cant_open_) {
      exit(-1);
    }
    return false;
  }
  return true;
}

}  // namespace mcr
}  // namespace art
