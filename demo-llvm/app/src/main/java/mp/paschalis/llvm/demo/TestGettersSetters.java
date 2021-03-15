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

/**
 * This test is mostly another TestSetters, with TestIncrement being an exception.
 * That method does a `++` on each field, meaning that it uses both getter and setter for it
 */
public class TestGettersSetters {

  boolean a = true;
  byte b = 4;
  char c = 'a';
  short d = 2;
  int e = 6;
  long f = 3;
  double g = 9.51;
  float h = 3.14f;

  boolean zz;
  byte bb;
  char cc;
  short ss;
  int ii;
  long jj;
  float ff;
  double dd;

  public static void RunTests() {
    TestGettersSetters t = new TestGettersSetters();
    t.SetBool();
    t.verifySetBool();
    t.verifyGetByte(t.GetByte());
    t.verifySetBool2(t.SetBool2());

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

    t.TestIncrement();
    t.verifyTestIncrement1();
    t.TestIncrement();
    t.verifyTestIncrement2();
  }

  byte GetByte() {
    return b;
  }

  public void verifyGetByte(byte bt) {
    CheckTest.Run("GetByte", b + "", (b == bt));
  }


  boolean SetBool2() {
    zz = true;
    return zz;
  }

  void SetBool() {
    zz = true;
  }

  public void verifySetBool2(boolean value) {
    CheckTest.Run("GetSet:SetBool2", zz + "", (zz == value));
  }

  public void verifySetBool() {
    CheckTest.Run("GetSet:SetBool", zz + "", (zz == true));
  }

  void SetByte() {
    bb = 4;
  }

  public void verifySetByte() {
    CheckTest.Run("GetSet:SetByte", bb + "", (bb == 4));
  }

  void SetChar() {
    cc = 'y';
  }

  public void verifySetChar() {
    CheckTest.Run("GetSet:SetChar", cc + "", (cc == 'y'));
  }

  void SetShort() {
    ss = 6;
  }

  public void verifySetShort() {
    CheckTest.Run("GetSet:SetShort", ss + "", (ss == 6));
  }

  void SetInt() {
    ii = 545;
  }

  public void verifySetInt() {
    CheckTest.Run("GetSet:SetInt", ii + "", (ii == 545));
  }


  void SetLong() {
    jj = 342354253445L;
  }

  public void verifySetLong() {
    CheckTest.Run("GetSet:SetLong", jj + "", (jj == 342354253445L));
  }


  void SetFloat() {
    ff = 34.3434f;
  }

  public void verifySetFloat() {
    CheckTest.Run("GetSet:SetFloat", ff + "", (ff == 34.3434f));
  }


  void SetDouble() {
    dd = 324.25234562346236326;
  }


  public void verifySetDouble() {
    CheckTest.Run("GetSet:SetDouble", dd + "", (dd == 324.25234562346236326));
  }

  // TODO OBJECT
  void TestIncrement() {
    a = !a;
    b++;
    c++;
    d++;
    e++;
    f++;
    g++;
    h++;
  }

  void verifyTestIncrement1() {

    // CheckTest.Run("GetSet:TestIncrement1:", ff + "", (ff == 34.3434f));
    CheckTest.Run("TestGettersSetters:TestIncrement1:a:",  a + "", (a == false));
    CheckTest.Run("TestGettersSetters:TestIncrement1:b:",  b + "", (b == 5));
    CheckTest.Run("TestGettersSetters:TestIncrement1:c:",  c + "", (c == 'b'));
    CheckTest.Run("TestGettersSetters:TestIncrement1:d:",  d + "", (d == 3));
    CheckTest.Run("TestGettersSetters:TestIncrement1:f:",  f + "", (f == 4));
    CheckTest.Run("TestGettersSetters:TestIncrement1:g:",  g + "", (g == 10.51));
    CheckTest.Run("TestGettersSetters:TestIncrement1:h:",  h + "", (h == 4.1400003f));
  }

  void verifyTestIncrement2() {
    CheckTest.Run("TestGettersSetters: TestIncrement2:a:", a + "", (a == true));
    CheckTest.Run("TestGettersSetters: TestIncrement2:b:", b + "", (b == 6));
    CheckTest.Run("TestGettersSetters: TestIncrement2:c:", c + "", (c == 'c'));
    CheckTest.Run("TestGettersSetters: TestIncrement2:d:", d + "", (d == 4));
    CheckTest.Run("TestGettersSetters: TestIncrement2:f:", f + "", (f == 5));
    CheckTest.Run("TestGettersSetters: TestIncrement2:g:", g + "", (g == 11.51));
    CheckTest.Run("TestGettersSetters: TestIncrement2:h:", h + "", (h == 5.1400003f));
  }

}
