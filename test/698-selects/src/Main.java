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

public class Main {
  public static int mZenMode = 0;

  public static int $noinline$foo(int internal, boolean check1, boolean check2) {
    int result = internal;
    if (check1) {
      // This block is to ensure `result` is a phi in the return block. Without this block
      // the compiler could just generate one block with selects.
      if (check2) {
        mZenMode = 42;
      }
      result = (internal == 1) ? 1 : 0;
    }
    // The optimization bug was to make the incorrect assumption that:
    //    phi = (internal, (internal == 1))
    // meant `internal` was a boolean.
    return result;
  }

  public static void main(String[] args) {
    int result = $noinline$foo(2, true, true);
    if (result != 0) {
      throw new Error("Expected 0, got " + result);
    }
  }
}
