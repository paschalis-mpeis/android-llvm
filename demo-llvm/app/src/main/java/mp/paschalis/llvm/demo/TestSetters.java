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

public class TestSetters {

  boolean z = true;
  byte b = 1;
  char c = 'a';
  short s = 2;
  int i = 2;
  long j = 2;
  double d = 1.324;
  float f = 1.77f;
  Human h;

  public static void RunTests() {
    TestSetters t = new TestSetters();
    t.SetBoolean();
    t.verifySetBoolean();
    t.SetByte();
    t.verifySetByte();
    t.SetChar();
    t.verifySetChar();
    t.SetShort();
    t.verifySetShort();
    t.SetInt();
    t.verifySetInt();
    t.SetLong();
    t.verifySetLong();
    t.SetFloat();
    t.verifySetFloat();
    t.SetDouble();
    t.verifySetDouble();

    t.SetObject();
    t.verifySetObject();
  }

  void SetBoolean() {
    z = false;
  }

  void SetByte() {
    b = 3;
  }

  void SetChar() {
    c = 'h';

  }

  void SetShort() {
    s = 6;
  }

  void SetInt() {
    i = 23235;
  }

  void SetLong() {
    j = 22521424234343L;
  }

  void SetFloat() {
    f = 234.3434f;
  }

  void SetDouble() {
    d = 24.3699234591243;
  }


  void SetObject() {
    h = new Human(5, 22);
  }

  public void verifySetBoolean() {
    CheckTest.Run("SetBoolean", z + "", (z == false));
  }

  public void verifySetByte() {
    CheckTest.Run("SetByte", b + "", (b == 3));
  }

  public void verifySetChar() {
    CheckTest.Run("SetChar", c + "", (c == 'h'));
  }

  public void verifySetShort() {
    CheckTest.Run("SetShort", s + "", (s == 6));
  }

  public void verifySetInt() {
    CheckTest.Run("SetInt", i + "", (i == 23235));
  }

  public void verifySetLong() {
    CheckTest.Run("SetLong", j + "", (j == 22521424234343L));
  }

  public void verifySetFloat() {
    CheckTest.Run("SetFloat", f + "", (f == 234.3434f));
  }

  public void verifySetDouble() {
    CheckTest.Run("SetDouble", d + "", (d == 24.3699234591243));
  }

  public void verifySetObject() {
    CheckTest.Run("SetObject", h.getRes() + "", (h.getRes() == 50));
  }

}
