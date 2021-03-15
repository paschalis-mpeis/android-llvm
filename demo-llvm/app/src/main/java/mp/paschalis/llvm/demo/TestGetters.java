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

public class TestGetters {

  boolean z = true;
  byte b = 4;
  char c = 'a';
  short s = 2;
  int i = 6;
  long j = 3;
  double d = 9.51;
  float f = 3.14f;
  Human h = new Human(11, 15);
  static Human hstatic = new Human(4, 5);

  public static void RunTests() {
    TestGetters t = new TestGetters();
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


  void TT() {
      getClass();

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
        CheckTest.Run("GetBoolean", z + "", (z == zz));
    }

    public void verifyGetByte(byte bb) {
        CheckTest.Run("GetByte", b + "", (b == bb));
  }

  public void verifyGetChar(char cc) {
    CheckTest.Run("GetChar", c + "", (c == cc));
  }

  public void verifyGetShort(short ss) {
    CheckTest.Run("GetShort", s + "", (s == ss));
  }

  public void verifyGetInt(int ii) {
    CheckTest.Run("GetInt", i + "", (i == ii));
  }

  public void verifyGetLong(long jj) {
    CheckTest.Run("GetLong", j + "", (j == jj));
  }

  public void verifyGetFloat(float ff) {
    CheckTest.Run("GetFloat", f + "", (f == ff));
  }

  public void verifyGetDouble(double dd) {
    CheckTest.Run("GetDouble", d + "", (d == dd));
  }

  public void verifyGetObject(Human hh) {
    CheckTest.Run("GetObject", h.getRes() + "", h.getRes() == hh.getRes());
  }

    public void verifyGetObjectStatic(Human hh) {
        CheckTest.Run("GetObjectStatic", hstatic.getRes() + "", hstatic.getRes() == hh.getRes());
    }

}
