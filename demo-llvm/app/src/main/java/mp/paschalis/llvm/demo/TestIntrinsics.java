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

public class TestIntrinsics {

  public static final String info = TestIntrinsics.class.getSimpleName();

  // TODO Math.sin(43);
  public static void RunTests() {
    TestIntrinsics t = new TestIntrinsics();
    // Math.min
    t.verifyMathMinF(t.MathMinF(345, 23.23423f));
    t.verifyMathMinD(t.MathMinD(21342.2134242, 232342334.234));
    t.verifyMathMinI(t.MathMinI(234, 34));
    t.verifyMathMinL(t.MathMinL(2341234234234L, 8234823482348L));
    // Math.max
    t.verifyMathMaxF(t.MathMaxf(345, 23.23423f));
    t.verifyMathMaxD(t.MathMaxD(21342.2134242, 232342334.234));
    t.verifyMathMaxI(t.MathMaxI(234, 34));
    t.verifyMathMaxL(t.MathMaxL(2341234234234L, 8234823482348L));

    t.verifyDouble("MathCos", t.MathCos(1024), 0.9873536182198484);

    // Math.abs
    t.verifyMathAbsl(t.MathAbsl(-13444));
    t.verifyMathAbsl(t.MathAbsl(13444));
    t.verifyMathAbsi(t.MathAbsi(-155));
    t.verifyMathAbsi(t.MathAbsi(155));
    t.verifyMathAbsf(t.MathAbsf(-3.542f));
    t.verifyMathAbsf(t.MathAbsf(3.542f));
    t.verifyMathAbsd(t.MathAbsd(-23535.25354354345));
    t.verifyMathAbsd(t.MathAbsd(23535.25354354345));

    // Math.round
    t.verifyMathRoundf(t.MathRoundf(45.235245234f));
    t.verifyMathRoundd(t.MathRoundd(234234234.635245234));

    // Math.sqrt
    t.verifyMathSqrt(t.MathSqrt(234.34534535));

    // Math.ceil
    t.verifyMathCeil(TestIntrinsics.MathCeil(14.6));
    // Math.floor
    t.verifyMathFloor(t.MathFloor(14.6));

    t.verifyIntBitsToFloat(t.IntBitsToFloat(236), 3.31E-43f);
    // t.verifyIntBitsToFloat(t.IntBitsToFloat(11), 1.5E-44f);
    // t.verifyIntBitsToFloat(t.IntBitsToFloat(263463), 3.6919E-40f);
    t.verifyDoubleToRawLongBits(t.doubleToRawLongBits(5.25d), 4617596992938311680L);

    String a = "abc";
    String b = "abc";
    t.verifyStringCompare(t.StringCompare(a, b), 0);
    String c = "abcd";
    t.verifyStringCompare(t.StringCompare(a, c), -1);
    t.verifyStringCompare(t.StringCompare(c, a), 1);

    verifyCharAt(charAt("asdfsdfasfd"));


    t.verifyThreadCurrentThread(t.ThreadCurrentThread());

    t.verifyIsInfinity("D:1", MathDoubleInfinity(345.345354d), false);
    t.verifyIsInfinity("D:2", MathDoubleInfinity(Double.POSITIVE_INFINITY), true);
    t.verifyIsInfinity("D:3", MathDoubleInfinity(Double.NEGATIVE_INFINITY), true);
    t.verifyIsInfinity("F:3", MathFloatInfinity(3.344f), false);
    t.verifyIsInfinity("F:3", MathFloatInfinity(Float.POSITIVE_INFINITY), true);
    t.verifyIsInfinity("F:3", MathFloatInfinity(Float.NEGATIVE_INFINITY), true);

    t.verifyTrailingZeros("I", t.CountTrailingZeros(0), 32);
    t.verifyTrailingZeros("I", t.CountTrailingZeros(5000), 3);
    t.verifyTrailingZeros("L", t.CountTrailingZeros(0L), 64);
    t.verifyTrailingZeros("L", t.CountTrailingZeros(500000L), 5);
  }


  static boolean MathDoubleInfinity(double a) {
    return Double.isInfinite(a);
  }

  static boolean MathFloatInfinity(float a) {
    return Float.isInfinite(a);
  }

  static double MathCeil(double a) {
    return Math.ceil(a);
  }

  float MathMinF(float a, float b) {
    return Math.min(a, b);
  }

  double MathMinD(double a, double b) {
    return Math.min(a, b);
  }

  int MathMinI(int a, int b) {
    return Math.min(a, b);
  }

  long MathMinL(long a, long b) {
    return Math.min(a, b);
  }

  float MathMaxf(float a, float b) {
    return Math.max(a, b);
  }

  double MathMaxD(double a, double b) {
    return Math.max(a, b);
  }

  int MathMaxI(int a, int b) {
    return Math.max(a, b);
  }

  long MathMaxL(long a, long b) {
    return Math.max(a, b);
  }

  double MathCos(double a) {
    return Math.cos(a);
  }

  double MathFloor(double a) {
    return Math.floor(a);
  }

  double MathRoundD(double a) {
    return Math.round(a);
  }

  int MathRoundF(float a) {
    return Math.round(a);
  }

  float IntBitsToFloat(int a) {
    return Float.intBitsToFloat(a);
  }

  long doubleToRawLongBits(double a) {
    return Double.doubleToRawLongBits(a);
  }

  int StringCompare(String a, String b) {
    return a.compareTo(b);
  }

  public void verifyStringCompare(int exec_res, int correct_result) {
    CheckTest.Run(info+": StringCompare: ", exec_res + "", (exec_res == correct_result));
  }

  public void verifyDoubleToRawLongBits(long exec_res, long correct_result) {
    CheckTest.Run(info+": DoubleToRawLongBits: ", exec_res + "", (exec_res == correct_result));
  }


  public void verifyIntBitsToFloat(float exec_res, float correct_result) {
    CheckTest.Run(info+": IntBitsToFloat: ", exec_res + "", (exec_res == correct_result));
  }

  public void verifyMathCeil(double res) {
    CheckTest.Run(info+": MathCeil", res + "", (res == 15.0));
  }

  public void verifyMathFloor(double res) {
    CheckTest.Run(info+": MathFloor", res + "", (res == 14.0));
  }

  public void verifyMathRoundD(long res) {
    CheckTest.Run(info+": MathRoundD", res + "", (res == 14));
  }

  public void verifyMathRoundF(int res) {
    CheckTest.Run(info+": MathRoundF", res + "", (res == 14));
  }


  public void verifyMathMinF(float res) {
    CheckTest.Run(info+": MathMin", res + "", (res == 23.23423f));
  }

  public void verifyMathMinD(double res) {
    CheckTest.Run(info+": MathMinDouble", res + "", (res == 21342.2134242));
  }

  public void verifyMathMinI(int res) {
    CheckTest.Run(info+": MathMinInt", res + "", (res == 34));
  }

  public void verifyMathMinL(long res) {
    CheckTest.Run(info +": MathMinLong", res + "", (res == 2341234234234L));
  }

  public void verifyMathMaxF(float res) {
    CheckTest.Run(info+": MathMaxFloat", res + "", (res == 345f));
  }

  public void verifyMathMaxD(double res) {
    CheckTest.Run(info +": MathMaxDouble", res + "", (res == 232342334.234));
  }

  public void verifyMathMaxI(int res) {
    CheckTest.Run(info+ ": MathMaxInt", res + "", (res == 234));
  }

  public void verifyMathMaxL(long res) {
    CheckTest.Run(info +": MathMaxLong", res + "", (res == 8234823482348L));
  }


  // TODO MathMind
  // TODO MathMini
  // TODO MathMinl


  long MathAbsl(long a) {
    return Math.abs(a);
  }

  int MathAbsi(int a) {
    return Math.abs(a);
  }

  float MathAbsf(float a) {
    return Math.abs(a);
  }

  double MathAbsd(double a) {
    return Math.abs(a);
  }


  public void verifyMathAbsl(long res) {
    CheckTest.Run(info +": MathAbsLong", res + "", (res == 13444));
  }

  public void verifyMathAbsi(int res) {
    CheckTest.Run(info + ": MathAbsInt", res + "", (res == 155));
  }


  public void verifyMathAbsf(float res) {
    CheckTest.Run(info +": MathAbsFloat", res + "", (res == 3.542f));
  }

  public void verifyMathAbsd(double res) {
    CheckTest.Run(info +": MathAbsDouble", res + "", (res == 23535.25354354345));
  }


  int MathRoundf(float a) {
    return Math.round(a);
  }

  long MathRoundd(double a) {
    return Math.round(a);
  }

  public void verifyMathRoundf(int res) {
    CheckTest.Run(info + ": MathRoundFloat", res + "", (res == 45));
  }

  public void verifyMathRoundd(long res) {
    CheckTest.Run(info +": MathRoundDouble", res + "", (res == 234234235));
  }

  Thread ThreadCurrentThread() {
    Thread t = Thread.currentThread();
    t.setName("mythread");

    return t;
  }

  public void verifyThreadCurrentThread(Thread t) {
    CheckTest.Run(info + ": ThreadCurrentThread", t.getName() + "", (t.getName().equals("mythread")));
  }

  public double MathSqrt(double a) {
    return Math.sqrt(a);
  }

  public void verifyMathSqrt(double res) {
    CheckTest.Run(info + ": Math.sqrt", res + "", (res == 15.308342344943819));
  }

  public static char charAt(String in) {
    return in.charAt(3);
  }
  public int CountTrailingZeros(int n) {
    return java.lang.Integer.numberOfTrailingZeros(n);
  }

  public int CountTrailingZeros(long n) {
    return java.lang.Long.numberOfTrailingZeros(n);
  }

  public void verifyIsInfinity(String ver, boolean res, boolean correct_res) {
    CheckTest.Run(info + ": Infinity:"+ver, res + "(c:"+correct_res+")", (res == correct_res));
  }

  public void verifyTrailingZeros(String ver, long res, long correct_res) {
    CheckTest.Run(info + ": TrailingZeros:"+ver, res + "(c:"+correct_res+")", (res == correct_res));
  }

  static void verifyCharAt(char res) {
    CheckTest.Run(info + ": CharAt:", res + "", (res == 'f'));
  }

  private void verifyDouble(String desc, double res, double correct) {
    CheckTest.Run(info + ": " + desc, res + "", correct+"", (res == correct));
  }
}
