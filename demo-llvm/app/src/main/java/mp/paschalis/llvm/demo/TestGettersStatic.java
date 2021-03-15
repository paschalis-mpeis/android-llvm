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

import mp.paschalis.llvm.demo.TestHelpers.Human;

public class TestGettersStatic {

  static boolean z = true;
  static byte b = 4;
  static char c = 'a';
  static short s = 2;
  static int i = 6;
  static long j = 3;
  static double d = 9.51;
  static float f = 3.14f;
  static Human h = new Human(5,3);


  public static void RunTests() {
    TestGettersStatic t = new TestGettersStatic();
    t.verifyGetStaticBoolean(t.GetStaticBoolean());
    t.verifyGetStaticByte(t.GetStaticByte());
    t.verifyGetStaticChar(t.GetStaticChar());
    t.verifyGetStaticShort(t.GetStaticShort());
    t.verifyGetStaticInt(t.GetStaticInt());
    t.verifyGetStaticLong(t.GetStaticLong());
    t.verifyGetStaticFloat(t.GetStaticFloat());
    t.verifyGetStaticDouble(t.GetStaticDouble());
    t.verifyGetStaticObject(t.GetStaticObject());
  }

  boolean GetStaticBoolean() {
    return z;
  }

  byte GetStaticByte() {
    return b;
  }

  char GetStaticChar() {
    return c;
  }

  short GetStaticShort() {
    return s;
  }

  int GetStaticInt() {
    return i;
  }

  long GetStaticLong() {
    return j;
  }

  float GetStaticFloat() {
    return f;
  }

  double GetStaticDouble() {
    return d;
  }

  Human GetStaticObject() {
    return h;
  }

  public void verifyGetStaticBoolean(boolean zz) {
    CheckTest.Run("GetStaticBoolean", z + "", (z == zz));
  }

  public void verifyGetStaticByte(byte bb) {
    CheckTest.Run("GetStaticByte", b + "", (b == bb));
  }

  public void verifyGetStaticChar(char cc) {
    CheckTest.Run("GetStaticChar", c + "", (c == cc));
  }

  public void verifyGetStaticShort(short ss) {
    CheckTest.Run("GetStaticShort", s + "", (s == ss));
  }

  public void verifyGetStaticInt(int ii) {
    CheckTest.Run("GetStaticInt", i + "", (i == ii));
  }

  public void verifyGetStaticLong(long jj) {
    CheckTest.Run("GetStaticLong", j + "", (j == jj));
  }

  public void verifyGetStaticFloat(float ff) {
    CheckTest.Run("GetStaticFloat", f + "", (f == ff));
  }

  public void verifyGetStaticDouble(double dd) {
    CheckTest.Run("GetStaticDouble", d + "", (d == dd));
  }

  public void verifyGetStaticObject(Human hh) {
    CheckTest.Run("GetStaticObject", h.getRes() + "", (h.getRes() == hh.getRes()));
  }

}
