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

public class TestDivNullCheck {

  static void RunTest(int n) {

    TestDivNullCheck t = new TestDivNullCheck();

    switch (n) {
      case 0:
        Log.i(TAG, "Calling: T1, T2, T6");
      t.verifyT1(t.T1());
      t.verifyT2(t.T2());
      t.verifyT6(t.T6());
      break;
      case 1:
        Log.e(TAG, "Calling: T0: die:static");
        t.T0(); // die:static VERIFIED
        break;
      case 2:
        Log.e(TAG, "Calling: T3: die:static");
        t.T3(); // die:static VERIFIED
        break;
      case 3:
        Log.e(TAG, "Calling: T4: die:dynamic");
        t.T4(0); // die:dynamic VERIFIED
        break;
      case 4:
        // maybe this is static..
        Log.e(TAG, "Calling: T5: die:static");
        t.T5(); // die:static VERIFIED
    }
  }


  public void verifyT1(float res) {
    CheckTest.Run("T1", res + "", (res == Float.POSITIVE_INFINITY));
  }

  public void verifyT2(double res) {
    CheckTest.Run("T2", res + "", (res == Float.POSITIVE_INFINITY));
  }

  public void verifyT6(long res) {
    CheckTest.Run("T6", res + "", (res == -493619368618686L));
  }


  int T0() {
    int x = 10;
    x++;
    if (x % 2 == 0) x++;

    return x / 0;
  }


  float T1() {
    float x = 10.0f;
    x++;
    if (x % 2 == 0) x++;

    return x / 0;
  }

  double T2() {
    double x = 10.23423510f;
    x++;
    if (x % 2 == 0) x++;

    return x / 0;
  }

  short T3() {
    short x = 9;
    if (x % 2 == 0) x++;

    return (short) (((short) x) / ((short) 0));
  }

  char T4(int divisor) {
    char x = 59;
    if (x % 2 == 0) x++;

    return (char) (((char) x) / ((char) divisor));
  }


  long T5() {
    long x = 5923432423424234L;
    if (x % 2 == 0) x++;
    return x / 0;
  }


  long T6() {
    long x = 5923432423424234L;
    long y = 123;
    for (int i = 0; i < 10; i++) {
      if (i % 2 == 0) {
        x++;
        y -= 23;
      } else {
        y -= 4;
      }
    }

    return x / y;
  }


}
