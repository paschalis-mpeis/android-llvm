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

public class TestLLVMtoQuickStatic {
  static final String info = TestLLVMtoQuickStatic.class.getSimpleName() + ": ";

  static boolean _z=false;
  static byte _b=3;
  static short _s=5;
  static int _i=34;
  static long _l=30;
  static double _d=34.344;
  static float _f=4.34f;
  static Human _h=new Human(3,5);

  static void RunTestsBasic() {
    TestLLVMtoQuickStatic t = new TestLLVMtoQuickStatic();

    CheckTest.RunCheck();
    t.RetIgnored1(5);
    CheckTest.RunCheckPassed("RetIgnored 1: DONE");
    CheckTest.RunCheck();
    t.RetIgnored2(5, 4);
    CheckTest.RunCheckPassed("RetIgnored 2: DONE");
    CheckTest.RunCheck();
    t.RetIgnored3(5, 343434,34);
    CheckTest.RunCheckPassed("RetIgnored 3: DONE");
    CheckTest.RunCheck();
    t.RetIgnored4(5.324f,34.34,34.34343433432219f);
    CheckTest.RunCheckPassed("RetIgnored 4: DONE");
  }

  static void RunTestsNested() {
    TestLLVMtoQuickStatic t = new TestLLVMtoQuickStatic();
    // More complicated tests
    verify("T1", t.T1(10, 5), 340);
    verify("T2", t.T2(5, 10), 2230);
    verify("T3", t.T3(5, 10), 2930);
    verify("T4", t.T4(5, 10), 1275);
    T5();
    T6();
  }

  int T1(int s, int j) {
    float r = 0;
    for (int i = 0; i < j + 10; i++) {
      float a, b, c;
      a = i + 0.3f;
      b = s + 0.3f;
      c = j + 0.1f;
      r+= inner1__NOT_HOT__(a, b, c);
    }
    return (int) r;
  }

  int T2(int s, int j) {
    int r = 0;
    for (int i = 0; i < j + 10; i++) {
      int a = i+1;
      int b = a+3;
      int c = i+j;
      int d = a+c;
      r+= inner2__NOT_HOT__(a, b, c, d, i-j);
    }
    return r;
  }

  int T3(int s, int j) {
    int r = 0;
    for (int i = 0; i < j + 10; i++) {
      int a = i+1;
      int b = a+3;
      r+=inner3__NOT_HOT__(i, a, b);
    }
    return r;
  }

  long T4(int j, long b) {
    int r = 0;
    for (int i = 0; i < j + 10; i++) {
      r+=inner3__NOT_HOT__(i, b, j+i+3);
    }
    return r;
  }

  static void verify(String info, int res, int correctRes) {
    CheckTest.Run("TestLLVMtoQuick: " + info, res + "", (res == correctRes));
  }

  static void verify(String info, long res, long correctRes) {
    CheckTest.Run("TestLLVMtoQuick: " + info, res + "", (res == correctRes));
  }

  static float inner1__NOT_HOT__(float a, float b, float c) {
    float r = a + b + c;
    Log.i(TAG, "inner1NH: res: " + r);
    return r;
  }

  static float inner2__NOT_HOT__(float s, float a, float b, int c, int d) {
    float r = s + a + b * 3 + c + d * 2;
    Log.i(TAG, "inner2NH: res: " + r +
      " input: s:" + s + " a:" + a + " b:" + b+
      " c:" + c + " d:" + d);
    return r;
  }

  static long inner3__NOT_HOT__(int a, long b, long c) {
    long  s=c;
    for (int i = 0; i < a; i++) {
      s+=b;
    }
    // Log.i(TAG, "inner3NH: res: " + s);
    return s;
  }

  void RetIgnored1(int x) {
    inner_ret_unused1__NOT_HOT__(x);
  }

  void RetIgnored2(int x, int y) {
    inner_ret_unused2__NOT_HOT__(x, y);
  }

  void RetIgnored3(int x, long a, int y) {
    inner_ret_unused3__NOT_HOT__(x, a, y);
  }

  void RetIgnored4(float a, double b, float c) {
    inner_ret_unused4__NOT_HOT__(a, c, b);
  }

  static int inner_ret_unused1__NOT_HOT__(int x) {
    x+=10;
    x+=0.3424334;
    Log.d(TAG, "Inner ignored: " + x);
    return x;
  }

  static int inner_ret_unused2__NOT_HOT__(int x, int y) {
    x+=10+y;
    x+=24334;
    x+=y;
    Log.d(TAG, "Inner ignored: " + x);
    return x;
  }

  static long  inner_ret_unused3__NOT_HOT__(int a, long b, int c) {
    long s=b+10+b;
    s+=0.3424334;
    s+=c;
    Log.d(TAG, "Inner ignored: " + s);
    return s;
  }

  static double inner_ret_unused4__NOT_HOT__(float a, float b, double c) {
    double s=b+10+b;
    s+=0.3424334;
    s+=c;
    Log.d(TAG, "Inner ignored: " + s);
    return s;
  }

  public static int T5() {
    // Needs LLVMtoQuick
    return Log.i(TAG, "T5: .......");
  }

  public static void T6() {
    // Needs LLVMtoQuick
    Log.i(TAG, "T6......");
  }

}
