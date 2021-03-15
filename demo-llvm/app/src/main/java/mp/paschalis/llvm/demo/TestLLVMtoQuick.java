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

public class TestLLVMtoQuick {
  static final String info = TestLLVMtoQuick.class.getSimpleName() + ": ";

  static void RunTestsBasic() {
    TestLLVMtoQuick t = new TestLLVMtoQuick();

    boolean z = t.RetBool(false);
    t.verifyRetBool(z, true);
    byte b = t.RetByte((byte) 4);
    t.verifyRetByte(b, (byte) 7);
    char c = t.RetChar('c');
    t.verifyRetChar(c, 'e');
    short s = t.RetShort((short) 4);
    t.verifyRetShort(s, (short) 8);
    int i = t.RetInt(34, 11);
    t.verifyRetInt(i, 68);
    long j = t.RetLong(232342);
    t.verifyRetLong(j, 2574685);
    float f = t.RetFloat(34.09f);
    t.verifyRetFloat(f, (float) 37.23);
    double d = t.RetDouble(324.23422344);
    t.verifyRetDouble(d, 559.217716724934);

    Human h = t.RetObject(10,34);
    t.verifyRetObject(h, 78);

    t.RetIgnored1(5);
    Log.d(TAG, "RetIgnored 1: DONE");
    t.RetIgnored2(5, 4);
    Log.d(TAG, "RetIgnored 3: DONE");
    t.RetIgnored3(5, 343434,34);
    Log.d(TAG, "RetIgnored 3: DONE");
    t.RetIgnored4(5.324f,34.34,34.34343433432219f);
    Log.d(TAG, "RetIgnored 4: DONE");

    // params
    verify("T1", t.T1(
      false, ((byte) 3), 'c', ((short) 4)), 106);
    verify("T2", t.T2(34, 34343, 16.33f, 4.345345), 34397);
  }

  static void RunTestsNested() {
    Log.i(TAG, "Running: T3i, T3, T4, T5, T6");
    TestLLVMtoQuick t = new TestLLVMtoQuick();
    verify("T3i", t.T3i(10, 5), 88);
    verify("T3", t.T3(10, 5), 340);
    verify("T4", t.T4(5, 10), 2230);
    verify("T5", t.T5(5, 10), 2930);
    verify("T6", t.T6(5, 10), 1275);
  }


  int T1(boolean z, byte b, char c, short s) {
    // BUG in mcr_analyser:
    // the method below is resolved to the pretty method:
    // void android.app.Dialog.<init>(android.content.Context, boolean, android.content.DialogInterface$OnCancelListener) // method@160
    /*
    This dex_file here must be wrong?
    std::string pretty_callee = caller_ref.dex_file->PrettyMethod(callee_idx);

     */
    return in1__NOT_HOT__(z, b, c, s);
  }

  int T2(int i, long l, float f, double d) {
    return in2__NOT_HOT__(i, l, f, d);
  }

  private int in1__NOT_HOT__(boolean z, byte b, char c, short s) {
    Log.i(TAG, "Boolean: " + z);
    Log.i(TAG, "Byte: " + b);
    Log.i(TAG, "Char: " + c);
    Log.i(TAG, "Short: " + s);
    return (int) ((int) b+c+s);
  }

  private int in2__NOT_HOT__(int i, long l, float f, double d) {
    Log.i(TAG, "Int: " + i);
    Log.i(TAG, "Long: " + l);
    Log.i(TAG, "Float: " + f);
    Log.i(TAG, "Double: " + d);
    // Log.i(TAG, "Human: " + h);
    return (int) ((int) i+l+f+d);
  }


  int T3(int s, int j) {
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

  int T3i(int s, int j) {
    float r = 0;
    //j+1: returns 109. works
    //j: returns 15, correct: 88
    for (int i = 0; i <j; i++) {
      float a, b, c;
      a = i + 0.3f;
      b = s + 0.3f;
      c = j + 0.1f;
      r+= inner1_hot(a, b, c);
      // r+= inner1__NOT_HOT__(a, b, c);
    }
    return (int) r;
  }



  int T4(int s, int j) {
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

  int T5(int s, int j) {
    int r = 0;
    for (int i = 0; i < j + 10; i++) {
      int a = i+1;
      int b = a+3;
      r+=inner3__NOT_HOT__(i, a, b);
    }
    return r;
  }

  long T6(int j, long b) {
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

  static float inner1_hot(float a, float b, float c) {
    float r = a + b + c;
    // Log.i(TAG, "inner1HOT: res: " + r);
    return r;
  }

  float inner1__NOT_HOT__(float a, float b, float c) {
    float r = a + b + c;
    // Log.i(TAG, "inner1_NOT_HOT: res: " + r);
    return r;
  }

  float inner2__NOT_HOT__(float s, float a, float b, int c, int d) {
    float r = s + a + b * 3 + c + d * 2;
    // Log.i(TAG, "inner2NH: res: " + r +
    //   " input: s:" + s + " a:" + a + " b:" + b+
    //   " c:" + c + " d:" + d);
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

  boolean RetBool(boolean x) {
    return RetBool__NOT_HOT__(x);
  }
  boolean RetBool__NOT_HOT__(boolean x) {
    return !x;
  }

  public void verifyRetBool(boolean res, boolean c) {
    CheckTest.Run(info + ": RetBool", res + "", (res == c));
  }

  byte RetByte(byte x) {
    return RetByte__NOT_HOT__(x);
  }
  byte RetByte__NOT_HOT__(byte x) {
    return (byte) (x+3);
  }

  public void verifyRetByte(byte res, byte c) {
    CheckTest.Run(info +"RetByte", res + "", (res == c));
  }

  char RetChar(char x) {
    return RetChar__NOT_HOT__(x);
  }

  char RetChar__NOT_HOT__(char x) {
    return (char) (x+(char)2);
  }

  public void verifyRetChar(char res, char c) {
    CheckTest.Run(info +"RetChar", res + "", (res == c));
  }

  short RetShort(short x) {
    return RetShort__NOT_HOT__(x);
  }

  short RetShort__NOT_HOT__(short x) {
    return (short) (4+x);
  }

  public void verifyRetShort(short res, short c) {
    CheckTest.Run(info +"RetShort", res + "", (res == c));
  }

  int RetInt(int x, int y) {
    x += 11;
    y += 12;
    return RetInt__NOT_HOT__(x, y);
  }

  int RetInt__NOT_HOT__(int x, int y) {
    return x + y;
  }

  public void verifyRetInt(int res, int c) {
    CheckTest.Run(info +"RetInt", res + "", (res == c));
  }

  long RetLong(long x) {
    return RetLong__NOT_HOT__(x);
  }

  long RetLong__NOT_HOT__(long x) {
    return 2342343+x;
  }

  public void verifyRetLong(long res, long c) {
    CheckTest.Run(info +"RetLong", res + "", (res == c));
  }
  float RetFloat(float f) {
    return RetFloat__NOT_HOT__(f);
  }

  float RetFloat__NOT_HOT__(float f) {
    return 3.14f +f;
  }

  public void verifyRetFloat(float res, float c) {
    CheckTest.Run(info +"RetFloat", res + "", (res == c));
  }

  double RetDouble(double x) {
    return RetDouble__NOT_HOT__(x);
  }
  double RetDouble__NOT_HOT__(double x) {
    return 234.983493284934 + x;
  }

  public void verifyRetDouble(double res, double c) {
    CheckTest.Run(info +"RetDouble", res + "", (res == c));
  }

  Human RetObject(int x, int y) {
    return RetObject__NOT_HOT__(x, y);
  }

  Human RetObject__NOT_HOT__(int x, int y) {
    x++; y+=10;
    return new Human(x, y);
  }

  public void verifyRetObject(Human h, int c) {
    Log.i(TAG,info+ "Height: " + h.getHeight() + "\nKilos: " + h.getKilos());
    CheckTest.Run(info+ "RetObject", h.getRes() + "", (h.getRes() == c));
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

  int inner_ret_unused1__NOT_HOT__(int x) {
    x+=10;
    x+=0.3424334;
    Log.d(TAG, "IgnoreInner1: " + x);
    return x;
  }

  int inner_ret_unused2__NOT_HOT__(int x, int y) {
    x+=10+y;
    x+=24334;
    x+=y;
    Log.d(TAG, "IgnoreInner2: " + x);
    return x;
  }

  long  inner_ret_unused3__NOT_HOT__(int a, long b, int c) {
    long s=b+10+b;
    s+=0.3424334;
    s+=c;
    Log.d(TAG, "IgnoreInner3: " + s);
    return s;
  }

  double inner_ret_unused4__NOT_HOT__(float a, float b, double c) {
    double s=b+10+b;
    s+=0.3424334;
    s+=c;
    Log.d(TAG, "IgnoreInner4: " + s);
    return s;
  }

}
