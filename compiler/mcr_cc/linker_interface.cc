/**
 * llvm-link interface
 *
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
#include "mcr_cc/linker_interface.h"

#include <sys/stat.h>
#include <fstream>
#include <queue>
#include "base/os.h"
#include "mcr_cc/mcr_cc.h"
#include "mcr_rt/filereader.h"
#include "mcr_rt/mcr_rt.h"
#include "mcr_rt/utils.h"

namespace art {
namespace mcr {

const std::string LinkerInterface::LINK = "llvm-link ";

std::set<std::string> LinkerInterface::dependencies_;

bool LinkerInterface::HasDependencies(std::string hf) {
  std::string link_filename = GetFileSrc(hf, FILE_DEPS_LINK);
  return OS::FileExists(link_filename.c_str());
}

void LinkerInterface::AddDependency(std::string caller, std::string callee) {
  if (!caller.compare(callee)) {
    D2LOG(INFO) << "AddDependency: skipping self: " << caller;
    return;
  }
  D4LOG(INFO) << "AddDependency: " << caller;
  dependencies_.insert(callee);
}

void LinkerInterface::StoreDependencies(std::string caller) {
  D3LOG(INFO) << "StoreDependencies";
  std::string link_filename = GetFileSrc(caller, FILE_DEPS_LINK);
  unlink(link_filename.c_str());

  if (dependencies_.size() > 0) {
    std::ofstream out(link_filename, std::ofstream::out);
    for (std::string dependency : dependencies_) {
      out << dependency << "\n";
    }
    out.close();
    chmod(link_filename.c_str(), 0666);
  }
}

/**
 * INFO we are already in the src dir by the llc_interface setup.
 */
int LinkerInterface::LinkMethods(std::string start_method) {
  D3LOG(INFO) << __func__ << ": " << start_method;
  std::string cmd;

  std::set<std::string> deps;
  deps.insert(GetFileSrc(start_method, McrCC::GetOuterBitcodeFilename()));
  deps.insert(GetFileSrc(start_method, McrCC::GetInnerBitcodeFilename()));

  if (LinkerInterface::HasDependencies(start_method)) {
    std::set<std::string> to_link(GetLinkMethods(start_method));
    D3LOG(INFO) << "LNK: Deps:" << to_link.size() << " : " << start_method;
    for (std::string to_link_method : to_link) {
      deps.insert(GetFileSrc(to_link_method, McrCC::GetInnerBitcodeFilename()));
    }
  }

  D3LOG(INFO) << __func__;
  std::string link_bitcodes;
  int cnt=0;
  for (std::string dep : deps) {
    cnt++;
    link_bitcodes += " " + dep;
    D3LOG(INFO) << dep;
  }

  cmd = LINK + link_bitcodes + spaced("-o") + HFlnkbc;
  cmd = McrCC::remove_extra_whitespaces(cmd);
#ifdef CRDEBUG3
  D2LOG(INFO) << "LNK: " << cmd;
  McrCC::LogLongMessage(cmd, WARNING);
#endif

  if (!EXE(cmd, true)) return 0;
  if (!CHMOD(HFlnkbc, "644")) return 0;

  return cnt;
}

std::set<std::string> LinkerInterface::GetLinkMethods(
    std::string start_method) {
  std::set<std::string> to_link;
  D3LOG(INFO) << __func__;

  CHECK(HasDependencies(start_method))
      << __func__ << ": " << start_method << " has no link dependencies.";

  std::vector<std::string> initial_deps =
      FileReader(__func__, GetFileSrc(start_method, FILE_DEPS_LINK)).GetData();

  std::queue<std::string> q;
  for (std::string dep : initial_deps) {
    D3LOG(INFO) << "Adding initial dep: " << dep;
    q.push(dep);
  }

  while (!q.empty()) {
    std::string to_link_method = q.front();
    q.pop();
    to_link.insert(to_link_method);
    D4LOG(INFO) << "To link: " << to_link_method;

    // visit their dependencies
    if (HasDependencies(to_link_method)) {
      std::vector<std::string> deps =
          FileReader(__func__, GetFileSrc(to_link_method, FILE_DEPS_LINK))
              .GetData();
      for (std::string dep : deps) {
        if (dep.compare(start_method) == 0) continue;
        if (to_link.find(dep) == to_link.end()) {
          q.push(dep);
        } else {
          D4LOG(INFO) << "dependency of: " << to_link_method
                      << " is already linked: dep: " << dep;
        }
      }
    }
  }
  return to_link;
}

}  // namespace mcr
}  // namespace art
