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

public class TestGettersVolatile {

    volatile boolean z = true;
    volatile byte b = 4;
    volatile char c = 'a';
    volatile short s = 2;
    volatile int i = 6;
    volatile long j = 3;
    volatile double d = 9.51;
    volatile float f = 3.14f;
    volatile Human h = new Human(11, 15);
    volatile static Human hstatic = new Human(4, 5);

  public static void RunTests() {
    TestGettersVolatile t = new TestGettersVolatile();
    t.verifyGetBoolean(t.GetBoolean());
    t.verifyGetByte(t.GetByte());
    t.verifyGetChar(t.GetChar());
    t.verifyGetShort(t.GetShort());
    t.verifyGetInt(t.GetInt());
    t.verifyGetLong(t.GetLong());
    t.verifyGetFloat(t.GetFloat());
    t.verifyGetDouble(t.GetDouble());
    t.verifyGetObject(t.GetObject());
    t.verifyGetObjectStatic(t.GetObjectStatic());
  }

  boolean GetBoolean() {
    return z;
  }

  byte GetByte() {
    return b;
  }

  char GetChar() {
    return c;
  }

  short GetShort() {
    return s;
  }

  int GetInt() {
    return i;
  }

  long GetLong() {
    return j;
  }

  float GetFloat() {
    return f;
  }

  double GetDouble() {
    return d;
  }

    Human GetObject() {
        return h;
    }
    Human GetObjectStatic() {
        return hstatic;
    }

    public void verifyGetBoolean(boolean zz) {
        CheckTest.Run("GetBooleanVolatile", z + "", (z == zz));
    }

    public void verifyGetByte(byte bb) {
        CheckTest.Run("GetByteVolatile", b + "", (b == bb));
  }

  public void verifyGetChar(char cc) {
    CheckTest.Run("GetCharVolatile", c + "", (c == cc));
  }

  public void verifyGetShort(short ss) {
    CheckTest.Run("GetShortVolatile", s + "", (s == ss));
  }

  public void verifyGetInt(int ii) {
    CheckTest.Run("GetIntVolatile", i + "", (i == ii));
  }

  public void verifyGetLong(long jj) {
    CheckTest.Run("GetLongVolatile", j + "", (j == jj));
  }

  public void verifyGetFloat(float ff) {
    CheckTest.Run("GetFloatVolatile", f + "", (f == ff));
  }

  public void verifyGetDouble(double dd) {
    CheckTest.Run("GetDoubleVolatile", d + "", (d == dd));
  }

  public void verifyGetObject(Human hh) {
    CheckTest.Run("GetObjectVolatile", h.getRes() + "", h.getRes() == hh.getRes());
  }

    public void verifyGetObjectStatic(Human hh) {
        CheckTest.Run("GetObjectStaticVolatile", hstatic.getRes() + "", hstatic.getRes() == hh.getRes());
    }

}
