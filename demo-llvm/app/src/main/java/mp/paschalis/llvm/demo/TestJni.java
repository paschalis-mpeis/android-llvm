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

public class TestJni {
  public static final String klass = TestJni.class.getSimpleName();

  static void RunTests() {
    TestJni t = new TestJni();
    int arr1[] = {0, 1, 2, 3, 4, 5};
    int arr2[] = {5, 10, 20, 30, 40, 50};

    Log.i(TAG, "Calling: " + klass + ".T1");
    long r1 = t.T1(10, arr1, arr2);
    Log.i(TAG, "Calling: " + klass + ".T2");
    long r2 = t.T1(100, arr1, arr2);
    Log.i(TAG, "Calling: " + klass + ".T3");
    long r3 = t.T1(500, arr1, arr2);

    t.verifyT1(".1",r1, 3200);
    t.verifyT1(".2",r2, 32000);
    t.verifyT1(".3",r3,160000);

    t.TestLog1("abcdef_msg1", "Message2");
    t.TestLog2(TAG, "LOG2_test");
  }

  long T1(int iters, int[] arr1, int[] arr2) {
    // XXX INFO crashes unless is on full capture
    long s = 0;
    for (int j = 0; j < iters; j++) {
      if(j%2 == 0) {
        System.arraycopy(arr2, 0, arr1, 0, arr2.length);
      } else {
        System.arraycopy(arr1, 0, arr2, 0, arr1.length);
      }
      s+= arr1[0] + arr2[0];
      for (int i = 0; i < arr1.length; i++) {
        s += arr1[i];
      }
      for (int i = 0; i < arr2.length; i++) {
        s += arr2[i];
      }
    }
    return s;
  }

  void TestLog1(String tag, String msg) {
    Log.i(TAG, msg);
    // Log.i(tag, msg);
  }

  void TestLog2(String msg1, String msg2) {
    Log.i(TAG, "Log message: '" + msg1 + "' and '" + msg2 + "'");
  }

  public void verifyT1(String info, long res, long outcome) {
    CheckTest.Run("TestJni:T1"+info, "Outcome: " + res,
      (res == outcome));
  }
}
