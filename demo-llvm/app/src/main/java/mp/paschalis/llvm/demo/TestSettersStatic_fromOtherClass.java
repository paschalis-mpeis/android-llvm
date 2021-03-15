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

public class TestSettersStatic_fromOtherClass {

  // TODO give different numbers to this, not to mess TestSettersStatic verification!

  public static void RunTests() {
    TestSettersStatic_fromOtherClass t = new TestSettersStatic_fromOtherClass();
    t.SetStatic_other_class_Boolean();
    t.verifySetStatic_other_class_Boolean();
    t.SetStatic_other_class_Byte();
    t.verifySetStatic_other_class_Byte();
    t.SetStatic_other_class_Char();
    t.verifySetStatic_other_class_Char();
    t.SetStatic_other_class_Short();
    t.verifySetStatic_other_class_Short();
    t.SetStatic_other_class_Int();
    t.verifySetStatic_other_class_Int();
    t.SetStatic_other_class_Long();
    t.verifySetStatic_other_class_Long();
    t.SetStatic_other_class_Float();
    t.verifySetStatic_other_class_Float();
    t.SetStatic_other_class_Double();
    t.verifySetStatic_other_class_Double();

    t.SetStatic_other_class_Object();
    t.verifySetStatic_other_class_Object();
  }

  void SetStatic_other_class_Boolean() {
    TestSettersStatic.z = false;
  }

  void SetStatic_other_class_Byte() {
    TestSettersStatic.b=3;
  }

  void SetStatic_other_class_Char() {
    TestSettersStatic.c = 'h';

  }

  void SetStatic_other_class_Short() {
    TestSettersStatic.s = 6;
  }

  void SetStatic_other_class_Int() {
    TestSettersStatic.i = 23235;
  }

  void SetStatic_other_class_Long() {
    TestSettersStatic.j = 22521424234343L;
  }

  void SetStatic_other_class_Float() {
    TestSettersStatic.f = 234.3434f;
  }

  void SetStatic_other_class_Double() {
    TestSettersStatic.d = 24.3699234591243;
  }

    void SetStatic_other_class_Object() {
    TestSettersStatic.h = new Human(53,23);
  }


  public void verifySetStatic_other_class_Boolean() {
    CheckTest.Run("SetStatic_other_class_Boolean", TestSettersStatic.z + "", (TestSettersStatic.z == false));
  }

  public void verifySetStatic_other_class_Byte() {
    CheckTest.Run("SetStatic_other_class_Byte", TestSettersStatic.b + "", (TestSettersStatic.b == 3));
  }

  public void verifySetStatic_other_class_Char() {
    CheckTest.Run("SetStatic_other_class_Char", TestSettersStatic.c + "", (TestSettersStatic.c == 'h'));
  }

  public void verifySetStatic_other_class_Short() {
    CheckTest.Run("SetStatic_other_class_Short", TestSettersStatic.s + "", (TestSettersStatic.s == 6));
  }

  public void verifySetStatic_other_class_Int() {
    CheckTest.Run("SetStatic_other_class_Int", TestSettersStatic.i + "", (TestSettersStatic.i == 23235));
  }

  public void verifySetStatic_other_class_Long() {
    CheckTest.Run("SetStatic_other_class_Long", TestSettersStatic.j + "", (TestSettersStatic.j == 22521424234343L));
  }

  public void verifySetStatic_other_class_Float() {
    CheckTest.Run("SetStatic_other_class_Float", TestSettersStatic.f + "", (TestSettersStatic.f == 234.3434f));
  }

  public void verifySetStatic_other_class_Double() {
    CheckTest.Run("SetStatic_other_class_Double", TestSettersStatic.d + "", (TestSettersStatic.d == 24.3699234591243));
  }

    public void verifySetStatic_other_class_Object() {
    CheckTest.Run("SetStatic_other_class_Object", TestSettersStatic.h.getRes() + "", (TestSettersStatic.h.getRes() == 99));
  }

}
