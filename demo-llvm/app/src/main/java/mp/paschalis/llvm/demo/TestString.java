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

public class TestString {
  public static String info = TestString.class.getSimpleName();

  public static void RunTests() {
    verifyT2(T2());
    T1();
    verifyStringLength(TestLength("2314234", "asdfasfsafsafasfasfasfdasfa"));
    verifyTestInit(TestInit("abc", 5));
    char c1 = CompressedChar("abcdefg", "abcdgkasjgkajsfk", 3);
    CheckTest.Run(info, "CompressedChar:c1: '" + c1 +"'", c1=='d');
    char c2 = CompressedChar("abcdefg", "mznxcvmzxnv", 4);
    CheckTest.Run(info, "CompressedChar:c2: '" +c2 +"'", c2=='e');
  }

  public static void T1() {
    // Needs LLVMtoQuick
    Log.i(TAG, "T1: this causes LLVM to call Quick, but LLVM outer HF haven't returned yet.");
  }

  public static String TestInit(String in, int x) {
    StringBuilder n = new StringBuilder(in);

    for(int i=0; i<x; i++) {
      n.append("_").append(i);
    }
    n.append("EXTRA");

    return n.toString();
  }

    static void verifyTestInit(String res) {
    CheckTest.Run("TestInit: ", res + "", (res.equals("abc_0_1_2_3_4EXTRA")));
  }


  public static int TestLength(String in1, String in2) {
    int i1 = in1.length();
    int i2 = in2.length();

    if (i1 > i2) return i1;

    return i2;
  }
  public static String T2() {
    return "STRING1";
  }


  static void verifyT2(String res) {
    CheckTest.Run("TestString: T2:", res + "", (res.equals("STRING1")));
  }
    static void verifyStringLength(int res) {
    CheckTest.Run("TestString: StringLength:", res + "", (res == 27));
  }

  static char CompressedChar(String s1, String s2, int loc) {
      char c1 = s1.charAt(loc);
      char c2 = s2.charAt(loc+ 1);

      if(c1<c2) {
        return c1;
      } else  {
        return c2;
      }
  }

}
