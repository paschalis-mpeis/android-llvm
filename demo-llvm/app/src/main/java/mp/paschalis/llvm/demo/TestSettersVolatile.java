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

public class TestSettersVolatile {

    volatile boolean z = true;
    volatile byte b = 1;
    volatile char c = 'a';
    volatile short s = 2;
    volatile int i = 2;
    volatile long j = 2;
    volatile double d = 1.324;
    volatile float f = 1.77f;
    volatile Human h;

  public static void RunTests() {
    TestSettersVolatile t = new TestSettersVolatile();
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
    CheckTest.Run("SetBooleanVolatile", z + "", (z == false));
  }

  public void verifySetByte() {
    CheckTest.Run("SetByteVolatile", b + "", (b == 3));
  }

  public void verifySetChar() {
    CheckTest.Run("SetCharVolatile", c + "", (c == 'h'));
  }

  public void verifySetShort() {
    CheckTest.Run("SetShortVolatile", s + "", (s == 6));
  }

  public void verifySetInt() {
    CheckTest.Run("SetIntVolatile", i + "", (i == 23235));
  }

  public void verifySetLong() {
    CheckTest.Run("SetLongVolatile", j + "", (j == 22521424234343L));
  }

  public void verifySetFloat() {
    CheckTest.Run("SetFloatVolatile", f + "", (f == 234.3434f));
  }

  public void verifySetDouble() {
    CheckTest.Run("SetDoubleVolatile", d + "", (d == 24.3699234591243));
  }

  public void verifySetObject() {
    CheckTest.Run("SetObjectVolatile", h.getRes() + "", (h.getRes() == 50));
  }

}
