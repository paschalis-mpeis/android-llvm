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

public class TestArguments {

    public static boolean x=false;
    public static void RunTests() {
        TestArguments t = new TestArguments();
        t.verifyArgBoolFalse(t.ArgBool(true));
        t.verifyArgBoolTrue(t.ArgBool(false));
        t.verifyArgByte(t.ArgByte((byte) 3));
        t.verifyArgChar(t.ArgChar('b'));
        t.verifyArgShort(t.ArgShort((short) 2));
        t.verifyArgInt(t.ArgInt(455));
        t.verifyArgLong(t.ArgLong(99843555));
        t.verifyArgFloat(t.ArgFloat(5.234f));
        t.verifyArgDouble(t.ArgDouble(5656.43434344));
        t.verifyTestAll(t.TestAll(true, (byte) 3, 23434.34, 34.45f, 2342344, 2344, 'c', (short) 7));
    }

  boolean ArgBool(boolean z) {
    return !z;
  }

  public void verifyArgBoolFalse(boolean res) {
    CheckTest.Run("ArgBool:F:", res + "", (res == false));
  }

  public void verifyArgBoolTrue(boolean res) {
    CheckTest.Run("ArgBool:T:", res + "", (res == true));
  }

  byte ArgByte(byte b) {
    return (byte) (b + 1);
  }

  public void verifyArgByte(byte res) {
    CheckTest.Run("ArgByte", res + "", (res == 4));
  }

  char ArgChar(char c) {
    return (char) (c + 3);
  }

  public void verifyArgChar(char res) {
    CheckTest.Run("ArgChar", res + "", (res == 'e'));
  }

  short ArgShort(short s) {
    return (short) (s + 4);
  }

  public void verifyArgShort(short res) {
    CheckTest.Run("ArgShort", res + "", (res == 6));
  }

  int ArgInt(int x) {
    return x + 2345;
  }

  public void verifyArgInt(int res) {
    CheckTest.Run("ArgInt", res + "", (res == 2800));
  }


  long ArgLong(long l) {
    return l + 1324342525;
  }

  public void verifyArgLong(long res) {
    CheckTest.Run("ArgLong", res + "", (res == 1424186080));
  }


  float ArgFloat(float f) {
    return f + 3.345f;
  }

  public void verifyArgFloat(float res) {
    CheckTest.Run("ArgFloat", res + "", (res == 8.579f));
  }


  double ArgDouble(double d) {
    return d - 344.44;
  }

  public void verifyArgDouble(double res) {
    CheckTest.Run("ArgDouble", res + "", (res == 5311.99434344));
  }


  long TestAll(boolean z, byte b, double d, float f, long l, int i, char c, short s) {
    long res = s;

    if (z) {
      res += 1;
    }
    res += l + i + c;
    res += f;
    res += d;
    return res;
  }

  public void verifyTestAll(long res) {
    CheckTest.Run("TestAll", res + "", (res == 2368263));
  }

}
