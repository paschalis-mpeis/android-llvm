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

public class TestLoops {

  int a = 0;
  long b = 0;
  long c_ = 11;

public static void RunDemo() {
  TestLoops t = new TestLoops();
  t.Loop5D();
  t.verifyLoop5D();
}

public static void RunTests() {
    TestLoops t = new TestLoops();
    t.Loop2D();
    t.verifyLoop2D();
    t.Loop5D();
    t.verifyLoop5D();

    t.verifyT1(t.T1((short) 34, true, 65.54f, 34.34534543, 'd', 56));
    t.verifyT2(t.T1((short) 9, false, 2.3461f, 209.34543, 'f', 12));
  }

  // TODO also check time!
  long T1(short a, boolean b, float f, double d, char c, int z) {
    long res = 44;
    for (int i = 0; i < 20; i++) {
      for (int j = 0; j < 50; j++) {
        for (int k = 0; k < 200; k++) {
          if (b) {
            for (int l = 0; l < 10; l++) {
              f += 2.21555;
            }
          }
          res = (long) ((2 * z) + f);
        }
        res += d;
      }
      res += a + c;
    }
    c_+= res;
    return c_;
  }

  void Loop2D() {
    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 10; j++) {
        a++;
      }
    }
  }

  void Loop5D() {
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        for (int k = 0; k < 10; k++) {
          for (int l = 0; l < 10; l++) {
            for (int m = 0; m < 10; m++) {
              b++;
            }
          }
        }
      }
    }
  }

  void verifyLoop2D() {
    CheckTest.Run("TestLoops: Loop2D:", a + "", (a == 50));
  }

  void verifyLoop5D() {
    CheckTest.Run("TestLoops: Loop5D:", b + "", (b == 100000));
  }

  void verifyT1(long res) {
    CheckTest.Run("TestLoops: verifyT1:", res + "", (res == 4446406));
  }

  void verifyT2(long res) {
    CheckTest.Run("TestLoops: verifyT2:", res + "", (res == 4446752));
  }

}
