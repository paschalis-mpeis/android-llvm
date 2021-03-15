/*
 * Copyright 2017 Paschalis Mpeis
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

public class TestTryCatch {
  static void RunTests() {
    TestTryCatch t = new TestTryCatch();
    Log.d(TAG, "TestTryCatch:");

    verifyT("T1", t.T1("33"), 33);
    verifyT("T1", t.T1("3f"), 3);

    verifyT("T2", t.T2(5,10), -1);
    verifyT("T2", t.T2(15,10), 150);
  }

  int T1(String s) {
    // TXXX DIes upon return
    int r = 0;
    try {
      if (s == "33") {
        r = 33;
      } else {
        // don't even throw..
        // r = 5;
        // throw new NumberFormatException("Wrong");
        throw new Exception("Wrong");
      }
      // TODO news LLVMtoQUICK
      // r = Integer.parseInt(s);
    } catch (Exception nfe) {
      r = 3;
    }
    return r;
  }

  int T2(int s, int j) {
    int res = 0;
    try {
      if(s>10) {
        for(int i=0; i<s; i++) {
          res+=j;
        }
      } else {
        throw new Exception("myexception");
      }

    } catch (Exception e)  {
      res = -1;
    }
    return res;
  }

  public static void verifyT(String info, int res, int correct_res) {
    CheckTest.Run("TestTryCatch", info + ": " + res, res == correct_res);
  }

}
