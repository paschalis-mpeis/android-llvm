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

import mp.paschalis.llvm.demo.TestHelpers.Human;

import static mp.paschalis.llvm.demo.Debug.TAG;

public class TestArrays {

  boolean _z[] = new boolean [10];
  byte  _b[] = new byte [10];
  char _c[] = new char [10];
  short _s[] = new short[10];
  int _i[] = new int[10];
  long _l[] = new long[10];
  float _f[] = new float[10];
  double _d[] = new double[10];

  double a[] = new double[10];
  long b[][] = new long[100][500];
  short c[][][] = new short[5][5][5];
  double d[] = new double[10];
  int e[][][][] = new int[5][10][10][10];
  long z[] = new long[10];

  Human h[] = new Human[10];

  static void RunTests() {
    TestArrays t = new TestArrays();
    t.verifyTestLength(t.TestLength());

    t.TestArraySetDynamic(); // must run before ArrayGet* tests
    t.verifyArrayGetDynamic(t.TestArrayGetDynamic());
    t.verifyTestGetStatic(t.TestArrayGetStatic());

    t.TestArraySetStatic();
    t.verifyTestSetStatic();

    t.T1(34.12345);
    t.verifyT1();

    t.T2(10);
    t.verifyT2(10);


    long r3 = t.T3((short) 9);
    t.verifyT3((short) 9, r3);

    t.T4(3444);
    t.verifyT4(3444);

    double r = t.T5();
    t.verifyT5(r);

    t.TestArraySetObject();
    t.verifyObject(t.TestArrayGetObjectSum());
  }

  int TestLength() {
    // VERIFIES: getter of object, HArrayLength
    return a.length;
  }

  void verifyTestLength(int len) {
    CheckTest.Run("TestArrays: Length", len + "", (len == 10));
  }

  void TestArraySetObject() {
    for (int i = 0; i < h.length; i++) {
      h[i] = new Human(i, i + 10);
    }
  }

  int TestArrayGetObjectSum() {
    int res = 0;
    for (int i = 0; i < h.length; i++) {
      res += h[i].getRes();
    }
    return res;
  }

  void TestArraySetDynamic() {
    for (int i = 0; i < z.length; i++) {
      z[i] = i + 13245;
    }
  }

  int TestArrayGetDynamic() {
    int sum = 0;
    for (int i = 0; i < z.length; i++) {
      sum += z[i];
    }
    return sum;
  }

  void TestArraySetStatic() {
    z[0] = 99;
  }

  void T1(double y) {
    for (int i = 0; i < a.length; i++) {
      a[i] = y;
    }
  }

  void T2(int y) {
    for (int i = 0; i < b.length; i++) {
      for (int j = 0; j < b[i].length; j++) {
        b[i][j] = y + i + j;
      }
    }
  }

  long T3(short y) {
    long res = 0;
    for (int i = 0; i < c.length; i++) {
      for (int j = 0; j < c[i].length; j++) {
        for (int k = 0; k < c[i][j].length; k++) {
          c[i][j][k] = y;
          res += i + j + k + y;
        }
      }
    }
    return res;
  }


  void T4(double y) {
    // static index check
    d[3] = y;
    // trick the compiler to treat i as a dynamic, not
    // statically known value
    for (int i = 0; i < 10; i++) {
      if (i == 4) {
        d[i] = y + 1;
      }
    }
  }

  double T5() {
    for (int i = 0; i < e.length; i++) {
      for (int j = 0; j < e[i].length; j++) {
        for (int k = 0; k < e[i][j].length; k++) {
          for (int l = 0; l < e[i][j][k].length; l++) {
            e[i][j][k][l] = i + j + k + l + 4;
          }
        }
      }
    }

    double sum = 0;
    for (int i = 0; i < e.length; i++) {
      for (int j = 0; j < e[i].length; j++) {
        for (int k = 0; k < e[i][j].length; k++) {
          for (int l = 0; l < e[i][j][k].length; l++) {
            sum += e[i][j][k][l];
          }
        }
      }
    }
    return sum;
  }

  public void verifyObject(int res) {
    CheckTest.Run("TestArrays:: Object:", "" + res, res == 420);

  }

  public void verifyT1() {
    String tname = "Verify: TestArrays:T1";
    boolean correct = true;
    for (int i = 0; i < a.length && correct; i++) {
      if (a[i] != 34.12345) correct = false;
      // Log.d(TAG, tname + ": a[" + i + "]=" + a[i]);
    }
    CheckTest.Run(tname, "[all verification]", correct);
  }

  public void verifyT2(int y) {
    String tname = "Verify: TestArrays:T2";
    boolean correct = true;
    boolean print_rows = false;

    String row;
    for (int i = 0; i < b.length; i++) { //  && correct
      row = "";
      for (int j = 0; j < b[i].length; j++) { //  && correct
        if (b[i][j] != y + i + j) correct = false;
        row += i + ":" + b[i][j] + " ";
      }

      if (print_rows) {
        Log.e(TAG, tname + "row: " + i + ": " + row);
      }
    }
    CheckTest.Run(tname, "[all verification]: ", correct);
  }

  public void verifyT3(short y, long res) {
    String tname = "Verify: TestArrays:T3";
    boolean correct = true;

    for (int i = 0; i < c.length; i++) {
      for (int j = 0; j < c[i].length; j++) {
        for (int k = 0; k < c[i][j].length; k++) {
          if (c[i][j][k] != y) {
            correct = false;
          }
        }
      }
    }
    CheckTest.Run(tname, "[array verification]", correct);
    CheckTest.Run(tname, "[result verification]: res: " + res, (res == 1875));
  }

  public void verifyT4(double y) {
    String tname = "TestArrays:T4";
    CheckTest.Run(tname, "[array Verification]: " + " d[3]=" + d[3], d[3] == y);
    CheckTest.Run(tname, " [array Verification]: " + " d[4]=" + d[4], d[4] == (y + 1));
  }

  public void verifyT5(double result) {
    String tname = "TestArrays:T5";
    CheckTest.Run(tname, "result: " + result, result == 97500.0);
  }

  void verifyArrayGetDynamic(int res) {
    CheckTest.Run("ArrayGetDynamic", res + "", (res == 132495));
  }

  long TestArrayGetStatic() {
    return z[3];
  }

  void verifyTestGetStatic(long res) {
    CheckTest.Run("ArrayGetStatic", res + "", (res == 13248));
  }

  void verifyTestSetStatic() {
    CheckTest.Run("ArraySetStatic", z[0] + "", (z[0] == 99));
  }

}
