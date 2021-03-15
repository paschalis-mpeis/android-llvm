/*
 * Copyright 2021 Paschalis Mpeis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package mp.paschalis.llvm.demo;

import android.util.Log;

import static mp.paschalis.llvm.demo.Debug.TAG;

/**
 *  TODO Exceptions are NOT supported!
 */
public class TestExceptions {

  static void RunTests() {
    // Run one test at a time bcz they throw Exceptions
    TestExceptions t = new TestExceptions();
    try {
      t.T1();
    } catch (Exception e) {
      Log.e(TAG, "Handling exception");
      e.printStackTrace();
    }
    t.T2();
  }

  void T1() throws Exception {
    throw new Exception("test");
  }

  /**
   * @android6: HGraph is not generated for methods that have any try blocks.
   */
  void T2() {
    try {
      Log.d(TAG, "Message within T2");
      throw new Exception("test");
    } catch (Exception e) {
      Log.d(TAG, "Message within T2: catch block");
      e.printStackTrace();
    }

    Log.d(TAG, "Message within T2: returning..");
  }
}
