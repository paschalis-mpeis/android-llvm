/*
 * Copyright (C) 2019 The Android Open Source Project
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
 */

#include "android-base/logging.h"
#include "base/locks.h"
#include "base/mutex.h"
#include "common_runtime_test.h"
#include "runtime.h"
#include "thread-current-inl.h"

namespace art {

class RuntimeTest : public CommonRuntimeTest {};

// Ensure that abort works with ThreadList locks held.

TEST_F(RuntimeTest, AbortWithThreadListLockHeld) {
  // This assumes the test is run single-threaded: do not start the runtime to avoid daemon threads.

  constexpr const char* kDeathRegex = "Skipping all-threads dump as locks are held";
  ASSERT_DEATH({
    // The regex only works if we can ensure output goes to stderr.
    android::base::SetLogger(android::base::StderrLogger);

    MutexLock mu(Thread::Current(), *Locks::thread_list_lock_);
    Runtime::Abort("Attempt to abort");
  }, kDeathRegex);
}


TEST_F(RuntimeTest, AbortWithThreadSuspendCountLockHeld) {
  // This assumes the test is run single-threaded: do not start the runtime to avoid daemon threads.

  constexpr const char* kDeathRegex = "Skipping all-threads dump as locks are held";
  ASSERT_DEATH({
    // The regex only works if we can ensure output goes to stderr.
    android::base::SetLogger(android::base::StderrLogger);

    MutexLock mu(Thread::Current(), *Locks::thread_suspend_count_lock_);
    Runtime::Abort("Attempt to abort");
  }, kDeathRegex);
}

}  // namespace art
