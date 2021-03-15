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

public class TestGettersStatic_FromOtherClass {

  public static void RunTests() {
    TestGettersStatic_FromOtherClass t = new TestGettersStatic_FromOtherClass();

    // The first method does a:
    t.verifyGetStatic_otheClass_Boolean(t.GetStatic_otheClass_Boolean());
    t.verifyGetStatic_otheClass_Byte(t.GetStatic_otheClass_Byte());

    t.verifyGetStatic_otheClass_Char(t.GetStatic_otheClass_Char());
    t.verifyGetStatic_otheClass_Short(t.GetStatic_otheClass_Short());
    t.verifyGetStatic_otheClass_Int(t.GetStatic_otheClass_Int());
    t.verifyGetStatic_otheClass_Long(t.GetStatic_otheClass_Long());
    t.verifyGetStatic_otheClass_Float(t.GetStatic_otheClass_Float());
    t.verifyGetStatic_otheClass_Double(t.GetStatic_otheClass_Double());
    t.verifyGetStatic_otheClass_Object(t.GetStatic_otheClass_Object());
  }

  boolean GetStatic_otheClass_Boolean() {
    return TestGettersStatic.z;
  }

  byte GetStatic_otheClass_Byte() {
    return TestGettersStatic.b;
  }

  char GetStatic_otheClass_Char() {
    return TestGettersStatic.c;
  }

  short GetStatic_otheClass_Short() {
    return TestGettersStatic.s;
  }

  int GetStatic_otheClass_Int() {
    return TestGettersStatic.i;
  }

  long GetStatic_otheClass_Long() {
    return TestGettersStatic.j;
  }

  float GetStatic_otheClass_Float() {
    return TestGettersStatic.f;
  }

  double GetStatic_otheClass_Double() {
    return TestGettersStatic.d;
  }

    Human GetStatic_otheClass_Object() {
    return TestGettersStatic.h;
  }

  public void verifyGetStatic_otheClass_Boolean(boolean zz) {
    CheckTest.Run("GetStatic_otheClass_Boolean", TestGettersStatic.z + "", (TestGettersStatic.z == zz));
  }

  public void verifyGetStatic_otheClass_Byte(byte bb) {
    CheckTest.Run("GetStatic_otheClass_Byte", TestGettersStatic.b + "", (TestGettersStatic.b == bb));
  }

  public void verifyGetStatic_otheClass_Char(char cc) {
    CheckTest.Run("GetStatic_otheClass_Char", TestGettersStatic.c + "", (TestGettersStatic.c == cc));
  }

  public void verifyGetStatic_otheClass_Short(short ss) {
    CheckTest.Run("GetStatic_otheClass_Short", TestGettersStatic.s + "", (TestGettersStatic.s == ss));
  }

  public void verifyGetStatic_otheClass_Int(int ii) {
    CheckTest.Run("GetStatic_otheClass_Int", TestGettersStatic.i + "", (TestGettersStatic.i == ii));
  }

  public void verifyGetStatic_otheClass_Long(long jj) {
    CheckTest.Run("GetStatic_otheClass_Long", TestGettersStatic.j + "", (TestGettersStatic.j == jj));
  }

  public void verifyGetStatic_otheClass_Float(float ff) {
    CheckTest.Run("GetStatic_otheClass_Float", TestGettersStatic.f + "", (TestGettersStatic.f == ff));
  }

  public void verifyGetStatic_otheClass_Double(double dd) {
    CheckTest.Run("GetStatic_otheClass_Double", TestGettersStatic.d + "", (TestGettersStatic.d == dd));
  }

    public void verifyGetStatic_otheClass_Object(Human hh) {
    CheckTest.Run("GetStatic_otheClass_Object", TestGettersStatic.h.getRes() + "", (TestGettersStatic.h.getRes() == hh.getRes()));
  }

}
