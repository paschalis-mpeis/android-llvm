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

import android.util.Log;

import mp.paschalis.llvm.demo.TestHelpers.Human;

import static mp.paschalis.llvm.demo.Debug.TAG;

public class TestReturnType {

  public static void RunTests() {
    TestReturnType t = new TestReturnType();
    boolean z = t.RetBool();
    t.verifyRetBool(z);
    byte b = t.RetByte();
    t.verifyRetByte(b);
    char c = t.RetChar();
    t.verifyRetChar(c);
    short s = t.RetShort();
    t.verifyRetShort(s);
    int i = t.RetInt();
    t.verifyRetInt(i);
    long j = t.RetLong();
    t.verifyRetLong(j);
    float f = t.RetFloat();
    t.verifyRetFloat(f);
    double d = t.RetDouble();
    t.verifyRetDouble(d);

    Human h = t.RetObject(5, 14);
      t.verifyRetObject(h);
      t.verifyRetObject(h);
      Log.i(TAG, "H:height: " + h.getHeight());
      Log.i(TAG, "H:kilos: " + h.getKilos());

    // XXX TODO enable these as well
    t.verifyUsingRetObject(t.UsingRetObject(5, 10));
//    t.verifyUsingRetObject_notHot(t.UsingRetObject_notHot(6,11));
  }

  public static void RunCaptureTest() {
    TestReturnType t = new TestReturnType();
    Human h = t.RetObject(5, 14);
    t.verifyRetObject(h);
    t.verifyRetObject(h);
    Log.i(TAG, "H:height: " + h.getHeight());
    Log.i(TAG, "H:kilos: " + h.getKilos());
  }

  boolean RetBool() {
    return false;
  }

  public void verifyRetBool(boolean res) {
    CheckTest.Run("RetBool", res + "", (res == false));
  }

  byte RetByte() {
    return 3;
  }

  public void verifyRetByte(byte res) {
    CheckTest.Run("RetByte", res + "", (res == 3));
  }

  char RetChar() {
    return 'd';
  }

  public void verifyRetChar(char res) {
    CheckTest.Run("RetChar", res + "", (res == 'd'));
  }

  short RetShort() {
    return 4;
  }

  public void verifyRetShort(short res) {
    CheckTest.Run("RetShort", res + "", (res == 4));
  }

  int RetInt() {
    int x, y;
    x = 11;
    y = 12;
    return x + y;
  }

  public void verifyRetInt(int res) {
    CheckTest.Run("RetInt", res + "", (res == 23));
  }


  long RetLong() {
    return 234234324;
  }

  public void verifyRetLong(long res) {
    CheckTest.Run("RetLong", res + "", (res == 234234324));
  }


  float RetFloat() {
    return 3.14f;
  }

  public void verifyRetFloat(float res) {
    CheckTest.Run("RetFloat", res + "", (res == 3.14f));
  }


  double RetDouble() {
    return 234.983493284934;
  }


  public void verifyRetDouble(double res) {
    CheckTest.Run("RetDouble", res + "", (res == 234.983493284934));
  }

  Human RetObject(int x, int y) {
    return new Human(x, y);
  }


  Human RetObject__NOT_HOT__(int x, int y) {
    return new Human(x, y);
  }

  int UsingRetObject_notHot(int x, int y) {
    Human h = RetObject__NOT_HOT__(x,y);
    return h.getRes();
  }


  int UsingRetObject(int x, int y) {
    Human h = RetObject(x,y);
    return h.getRes();
  }


  int T1(short a, short b) {
    int res = b;
    for (int i = 0; i < a; i++) {
      res += i;
    }
    return res;
  }

  public void verifyRetObject(Human h) {
      Log.i(TAG,"Height: " + h.getHeight() + "\nKilos: " + h.getKilos());
      CheckTest.Run("RetObject", h.getRes() + "", (h.getRes() == 42));
  }

  public void verifyUsingRetObject(int res) {
    CheckTest.Run("UsingRetObject", res + "", (res == 38));
  }

  public void verifyUsingRetObject_notHot(int res) {
    CheckTest.Run("UsingRetObjectNotHot", res + "", (res == 37));
  }

}

