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

public class TestCasting {

  public static void RunTests() {
    TestCasting t = new TestCasting();
    t.verifyT1(t.T1());
    t.verifyT2(t.T2());
    t.verifyT3(t.T3());
    t.verifyT4(t.T4());
    t.verifyT5(t.T5());
    t.verifyT6(t.T6());
    t.verifyT7(t.T7());
    t.verifyT8(t.T8());
    t.verifyT9(t.T9());
    t.verifyT10(t.T10());
    t.verifyT11(t.T11());
  }

  // one type is 64 bits
  // If double then would be illegal unless casted
  // EXPECT: long casted to float (which is b, and return type)
  float T1() {
    long a = 234234224L;
    float b = 234.3434f;
    return a + b;
  }

  public void verifyT1(float res) {
    CheckTest.Run("TestCasting:T1", res + "", (res == 234234468L));
  }

  // int = int + float:
  // result MUST be integral.
  // What to expect: do FP operation. then cast result to int
  int T2() {
    int a = 234234;
    float b = 234.3434f;
    return (int) (a + b);
  }

  public void verifyT2(int res) {
    CheckTest.Run("TestCasting:T2", res + "", (res == 234468));
  }

  byte T3() {
    byte a = (byte) 125; // Byte.MAX_VALUE = 127
    byte b = (byte) 10;
    return (byte) (a + b);
  }

  public void verifyT3(byte res) {
    CheckTest.Run("TestCasting:T3", res + "", (res == -121));
  }

  int T4() {
    byte a = (byte) 125; // Byte.MAX_VALUE = 127
    byte b = (byte) 10;
    return a + b;
  }

  public void verifyT4(int res) {
    CheckTest.Run("TestCasting:T4", res + "", (res == 135));
  }

  boolean T5() {
    char a = '\uffff';
    char b = 'c';
    return a > b;
  }

  public void verifyT5(boolean res) {
    CheckTest.Run("TestCasting:T5", res + "", (res == true));
  }

  char T6() {
    char a = ('\uffff' - 3);
    char b = 'm';
    return (char) (a + b);
  }

  public void verifyT6(char res) {
    CheckTest.Run("TestCasting:T6", "'" + res + "'" + "", (res == 'i'));
  }

  float T7() {
    long a = 314;
    int b = 234234234;
    return a + b;
  }

  public void verifyT7(float res) {
    CheckTest.Run("TestCasting:T7", res + "", (res == 234234548f));
  }

  float T8() {
    int a = 314;
    float b = 234234234;
    return a + b;
  }

  public void verifyT8(float res) {
    CheckTest.Run("TestCasting:T8", res + "", (res == 234234568f));
  }

  double T9() {
    long a = 314;
    float b = 234234234;
    return a + b;
  }

  public void verifyT9(double res) {
    CheckTest.Run("TestCasting:T9", res + "", (res == 234234568f));
  }

  // Small types returning to bigger type
  int T10() {
    int a = 2147483640;  // Integer.MAX_VALUE;
    int b = 234;
    return a + b;
  }

  public void verifyT10(long res) {
    CheckTest.Run("TestCasting:T10", res + "", (res == -2147483422));
  }

  // Small types returning to bigger type
  long T11() {
    int a = 2147483640;  // Integer.MAX_VALUE;
    int b = 234;
    return a + b;
  }

  public void verifyT11(long res) {
    CheckTest.Run("TestCasting:T11", res + "", (res == -2147483422));
  }

}
