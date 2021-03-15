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

public class TestSettersStatic {

  static boolean z = true;
  static byte b=1;
  static char c='a';
  static short s=2;
  static int i=2;
  static long j=2;
  static double d=1.324;
  static float f=1.77f;
  static Human h;

  public static void RunTests() {
    TestSettersStatic t = new TestSettersStatic();
    t.SetStaticBoolean();
    t.verifySetStaticBoolean();
    t.SetStaticByte();
    t.verifySetStaticByte();
    t.SetStaticChar();
    t.verifySetStaticChar();
    t.SetStaticShort();
    t.verifySetStaticShort();
    t.SetStaticInt();
    t.verifySetStaticInt();
    t.SetStaticLong();
    t.verifySetStaticLong();
    t.SetStaticFloat();
    t.verifySetStaticFloat();
    t.SetStaticDouble();
    t.verifySetStaticDouble();

    t.SetStaticObject();
    t.verifySetStaticObject();
  }

  void SetStaticBoolean() {
    z = false;
  }

  void SetStaticByte() {
    b=3;
  }

  void SetStaticChar() {
    c = 'h';

  }

  void SetStaticShort() {
    s = 6;
  }

  void SetStaticInt() {
    i = 23235;
  }

  void SetStaticLong() {
    j = 22521424234343L;
  }

  void SetStaticFloat() {
    f = 234.3434f;
  }

  void SetStaticDouble() {
    d = 24.3699234591243;
  }


  void SetStaticObject() {
    h = new Human(34, 60);
  }

  public void verifySetStaticBoolean() {
    CheckTest.Run("SetStaticBoolean", z + "", (z == false));
  }

  public void verifySetStaticByte() {
    CheckTest.Run("SetStaticByte", b + "", (b == 3));
  }

  public void verifySetStaticChar() {
    CheckTest.Run("SetStaticChar", c + "", (c == 'h'));
  }

  public void verifySetStaticShort() {
    CheckTest.Run("SetStaticShort", s + "", (s == 6));
  }

  public void verifySetStaticInt() {
    CheckTest.Run("SetStaticInt", i + "", (i == 23235));
  }

  public void verifySetStaticLong() {
    CheckTest.Run("SetStaticLong", j + "", (j == 22521424234343L));
  }

  public void verifySetStaticFloat() {
    CheckTest.Run("SetStaticFloat", f + "", (f == 234.3434f));
  }

  public void verifySetStaticDouble() {
    CheckTest.Run("SetStaticDouble", d + "", (d == 24.3699234591243));
  }

    public void verifySetStaticObject() {
    CheckTest.Run("SetStaticObject", h.getRes() + "", (h.getRes() == 117));
  }

}
