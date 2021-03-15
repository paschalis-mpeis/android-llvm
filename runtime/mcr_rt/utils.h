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
#ifndef ART_RUNTIME_MCR_UTILS_H_
#define ART_RUNTIME_MCR_UTILS_H_

#include <string>
#include <vector>
#include "dex/primitive.h"

#define PATH "PATH"
#define PATH_LIB "LD_LIBRARY_PATH"

namespace art {
namespace mcr {

std::string PrettyPrimitive(Primitive::Type type);
void checkFileExists(std::string file);
void checkFolderExists(std::string folder);
void CheckDirExists(std::string dir);

std::string GetDirAppHfSrc(std::string shf);

std::string GetFileApp(std::string filename);
std::string GetFileAppHf(std::string hf, std::string filename);
std::string GetFileSrc(std::string hf, std::string filename);
std::string GetFileAppHfImg(std::string filename);

const char* StripHf(std::string hf);
std::vector<std::string> SplitString(const std::string& s, char delim);
std::string __HashCode(const char* str);

void AppendEnvVar(std::string, std::string);
std::string GetEnvVar(std::string const& key);

void vlog(const char* file, const int line,
    const char* caller, int art_severity, const char* tag,
    const char* format, va_list args);

}  // namespace mcr
uint64_t ProcessCpuNanoTime();
}  // namespace art

#endif  // ART_RUNTIME_MCR_UTILS_H_
