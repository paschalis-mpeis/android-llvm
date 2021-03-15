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

public class TestFloatingPoint {
  double a = 11.235;
  double b = 9.3456;
  float c = 3.45f;


  static void RunTests() {
    TestFloatingPoint testFloatingPoint = new TestFloatingPoint();
    testFloatingPoint.T1();
    testFloatingPoint.verifyT1();

    testFloatingPoint.T2(393.5, 54.2f);
    testFloatingPoint.verifyT2();

    testFloatingPoint.T3((short) 6, true, (byte) 2);
    testFloatingPoint.verifyT3();
  }


  void T1() {
    a += 34.534;
  }


  void T2(double x, float z) {
    b += x;
    b += z * 10;
  }

  void T3(short x, boolean t, byte b) {
    c += (x * 2);
    if (t) {
      c -= b;
    }
  }

  public void verifyT1() {
    CheckTest.Run("TestFloatingPoint:T1:", a + "", (a == 45.769));
  }

  public void verifyT2() {
    CheckTest.Run("TestFloatingPoint:T2:", b + "", (b == 944.8456));
  }

  public void verifyT3() {
    CheckTest.Run("TestFloatingPoint:T3:", c + "", (c == 13.45f));
  }

}
