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

public class TestOutOfBounds {

  public int a[] = new int[10];

  static void RunTest(int n) {
    TestOutOfBounds t = new TestOutOfBounds();
    switch (n) {
      case 0:
        // These pass check
        Log.i(TAG, "Calling: T2, T3, T4");
        t.runT2(); // VERIFIED
        t.runT3(); // VERIFIED
        t.runT4(); // VERIFIED
        break;
      case 1:
        Log.e(TAG, "Calling: runT0: dies");
        t.runT0(); // dies
        break;
      case 2:
        Log.e(TAG, "Calling: runT1: dies");
        t.runT1();
        break;
      case 3:
        Log.e(TAG, "Calling: runT5: dies");
        t.runT5(); // VERIFIED
      case 4:
        Log.e(TAG, "Calling: runT6: dies");
        t.runT6();
    }
  }

  void T0() {
    a[-5] = 5;
  }

  void runT0() {
    T0();
    Log.i(TAG, "TestOutOfBounds.T0: PASSED (should fail)");
  }

  void T1() {
    a[-1] = 5;
  }

  void runT1() {
    T1();
    Log.i(TAG, "TestOutOfBounds.T1: PASSED (should fail)");
  }

  void T2() {
    a[0] = 5;
  }
  void runT2() {
    CheckTest.RunCheck();
    T2();
    CheckTest.RunCheckPassed("TestOutOfBounds.T2: PASSED");
  }

  void T3() {
    a[1] = 5;
  }

  void runT3() {
    CheckTest.RunCheck();
    T3();
    CheckTest.RunCheckPassed("TestOutOfBounds.T3: PASSED");
  }

  void T4() {
    a[9] = 5;
  }

  void runT4() {
    CheckTest.RunCheck();
    T4();
    CheckTest.RunCheckPassed("TestOutOfBounds.T4: PASSED");
  }

  void T5() {
    a[10] = 5;
  }
  void runT5() {
    T5();
    Log.i(TAG, "TestOutOfBounds.T5: PASSED (should fail)");
  }
  void T6() {
    a[11] = 5;
  }

  void runT6() {
    T6();
    Log.i(TAG, "TestOutOfBounds.T6: PASSED (should fail)");
  }

}
