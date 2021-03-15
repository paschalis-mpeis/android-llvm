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

public class TestArrayGetSet {
  private static final String name = TestArrayGetSet.class.getSimpleName();

  boolean zz[] = new boolean [5];
  byte  bb[] = new byte [5];
  char cc[] = new char [5];
  short ss[] = new short[5];
  int ii[] = new int[5];
  long ll[] = new long[5];
  float ff[] = new float[5];
  double dd[] = new double[5];
  Human oo[] = new Human[5];

  boolean dbg_ = false;

  static void RunTests() {
    // INFO Call dynamic before static
    // Setters
    TestArrayGetSet t = new TestArrayGetSet();
    // Boolean
    t.SetDynamicZ(true);
    t.verifyZ("Dynamic", t.zz,5);
    t.SetStaticZ(false);
    t.verifyZ("Static", t.zz,3);
    // Byte
    t.SetDynamicB((byte)4);
    t.verifyB("Dynamic", t.bb,20);
    t.SetStaticB((byte)2);
    t.verifyB("Static", t.bb,16);
    // Char
    t.SetDynamicC('y');
    t.verifyC("Dynamic", t.cc,"yyyyy");
    t.SetStaticC('o');
    t.verifyC("Static", t.cc,"ooyyy");
    // Short
    t.SetDynamicS((short)7);
    t.verifyS("Dynamic", t.ss,35);
    t.SetStaticS((short)12);
    t.verifyS("Static", t.ss,45);
    // int
    t.SetDynamicI(5);
    t.verifyI("Dynamic", t.ii,25);
    t.SetStaticI(10);
    t.verifyI("Static", t.ii,35);
    // long
    t.SetDynamicL(53434);
    t.verifyL("Dynamic", t.ll,267170);
    t.SetStaticL(10000340);
    t.verifyL("Static", t.ll,20160982);
    // float
    t.SetDynamicF(3.345f);
    t.verifyF("Dynamic", t.ff,16.725000143051147);
    t.SetStaticF(14.34f);
    t.verifyF("Static", t.ff,38.71500039100647);
    // double
    t.SetDynamicD(45.3453453245345);
    t.verifyD("Dynamic", t.dd,226.7267266226725);
    t.SetStaticD(122.334535354);
    t.verifyD("Static", t.dd,380.70510668160347);
    // Object
    t.SetDynamicO(new Human(5,55));
    t.verifyO("Dynamic", t.oo,415);
    t.SetStaticO(new Human(22,4));
    t.verifyO("Static", t.oo,347);
    t.SetDynamicOsimple(new Human(55,45), 3);
    t.verifyO("DynamicIndex", t.oo,387);
    // Getters (static and dynamic)
    int i=0;
    t.verifySet("Z", t.GetBothZ(i++%5, i++%5), 1);
    t.verifySet("B", t.GetBothB(i++%5, i++%5), 14);
    t.verifySet("C", t.GetBothC(i++%5, i++%5), "yooy");
    t.verifySet("S", t.GetBothS(i++%5, i++%5), 38);
    t.verifySet("I", t.GetBothI(i++%5, i++%5), 25);
    t.verifySet("L", t.GetBothL(i++%5, i++%5), 30054454);
    t.verifySet("F", t.GetBothF(i++%5, i++%5), 24.37500023841858);
    t.verifySet("D", t.GetBothD(i++%5, i++%5), 335.359761357069);
    t.verifySet("O", t.GetBothO(i++%5, i++%5), 264);
    // Setters: null
    t.SetStaticONull();
    t.verifyO("StaticNull", t.oo,20221);
    t.SetDynamicONull(4);
    t.verifyO("DynamicNull", t.oo,30172);
  }

  // boolean
  void SetDynamicZ(boolean val) { for(int i =0; i<bb.length; i++) zz[i] = val; }
  void SetStaticZ(boolean val) { zz[0] = val; zz[1] = val; }
  long GetBothZ(int i, int j) {
    long sum=0;
    if(zz[i]) sum++;
    if(zz[j]) sum++;
    if(zz[1]) sum++;
    if(zz[3]) sum++;
    return sum;
  }
  // Byte
  void SetDynamicB(byte val) { for(int i =0; i<ii.length; i++) bb[i] = val; }
  void SetStaticB(byte val) { bb[0] = val; bb[1] = val; }
  long GetBothB(int i, int j) {
    long sum=0;
    sum+=bb[i];
    sum+=bb[j];
    sum+=bb[1];
    sum+=bb[2];
    return sum;
  }
  // char
  void SetDynamicC(char val) { for(int i =0; i<cc.length; i++) cc[i] = val; }
  void SetStaticC(char val) { cc[0] = val; cc[1] = val; }
  String GetBothC(int i, int j) {
    String sum="";
    sum+=cc[i];
    sum+=cc[j];
    sum+=cc[1];
    sum+=cc[2];
    return sum;
  }
  // Short
  void SetDynamicS(short val) { for(int i =0; i<ss.length; i++) ss[i] = val; }
  void SetStaticS(short val) { ss[0] = val; ss[1] = val; }
  long GetBothS(int i, int j) {
    long sum=0;
    sum+=ss[i];
    sum+=ss[j];
    sum+=ss[1];
    sum+=ss[2];
    return sum;
  }
  // int
  void SetDynamicI(int val) { for(int i =0; i<ii.length; i++) ii[i] = val; }
  void SetStaticI(int val) { ii[0] = val; ii[1] = val; }
  long GetBothI(int i, int j) {
    long sum=0;
    sum+=ii[i];
    sum+=ii[j];
    sum+=ii[1];
    sum+=ii[2];
    return sum;
  }
  // long
  void SetDynamicL(long val) { for(int i =0; i<ll.length; i++) ll[i] = val; }
  void SetStaticL(long val) { ll[0] = val; ll[1] = val; }
  long GetBothL(int i, int j) {
    // If in a loop (and not 2 arguments) for dynamic it requires:
    // VecSetScalars
    // VisitVecReduce
    // VisitVecExtractScalar
    long sum=0;
    sum+=ll[i];
    sum+=ll[j];
    sum+=ll[1];
    sum+=ll[4];
    return sum;
  }
  // Float
  void SetDynamicF(float val) { for(int i =0; i<ff.length; i++) ff[i] = val; }
  void SetStaticF(float val) { ff[0] = val; ff[1] = val; }
  double GetBothF(int i, int j) {
    double sum=0;
    sum+=ff[i];
    sum+=ff[j];
    sum+=ff[1];
    sum+=ff[2];
    return sum;
  }
  // Double
  void SetDynamicD(double val) { for(int i =0; i<dd.length; i++) dd[i] = val; }
  void SetStaticD(double val) { dd[0] = val; dd[1] = val; }
  double GetBothD(int i, int j) {
    double sum=0;
    sum+=dd[i];
    sum+=dd[j];
    sum+=dd[1];
    sum+=dd[2];
    return sum;
  }
  // Object: Human
  void SetDynamicO(Human val) { for(int i =0; i<ii.length; i++) oo[i] = val; }
  void SetDynamicOsimple(Human val, int idx) { oo[idx] = val; }
  void SetStaticO(Human val) { oo[2] = val; oo[4] = val; }
  void SetStaticONull() { oo[0] = null; oo[1] = null; }
  void SetDynamicONull(int idx) { oo[idx] = null; }
  long GetBothO(int i, int j) {
    long sum=0;
    sum+=oo[i].getRes();
    sum+=oo[j].getRes();
    sum+=oo[1].getRes();
    sum+=oo[2].getRes();
    return sum;
  }

  // Verification methods
  public void verifySet(String info, long res, long correctRes) {
    CheckTest.Run(name + ":Get" + info, res + " (C:" + correctRes+")", (res == correctRes));
  }
  public void verifySet(String info, double res, double correctRes) {
    CheckTest.Run(name + ":Get" + info, res + " (C:" + correctRes+")", (res == correctRes));
  }
  public void verifySet(String info, String res, String correctRes) {
    CheckTest.Run(name + ":Get" + info, res + " (C:" + correctRes+")", (res.equals(correctRes)));
  }

  public void verifyZ(String info, boolean a[], long correctRes) {
    long res = 0;
    for(int i =0; i<a.length; i++) {
      if(a[i]) res++;
    }
    CheckTest.Run(name + ":" + info + "Z", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyB(String info, byte a[], long correctRes) {
    long res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info + "B", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyC(String info, char a[], String correctRes) {
    String res = "";
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info + "C", res + " (C:" + correctRes+")", (res.equals(correctRes)));
  }

  public void verifyS(String info, short a[], long correctRes) {
    long res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info + "S", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyI(String info, int a[], long correctRes) {
    long res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info+"I", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyL(String info, long a[], long correctRes) {
    long res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info+"L", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyF(String info, float a[], double correctRes) {
    double res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info+"F", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyD(String info, double a[], double correctRes) {
    double res = 0;
    for(int i =0; i<a.length; i++) {
      res+=a[i];
    }
    CheckTest.Run(name + ":" + info+"D", res + " (C:" + correctRes+")", (res == correctRes));
  }

  public void verifyO(String info, Human a[], long correctRes) {
    long res = 0;
    if(a==null) {
      Log.e(TAG, "Whole array is null!");
    } else {
      for (int i = 0; i < a.length; i++) {
        if (a[i] == null) {
          if(dbg_) Log.w(TAG, "idx:"+i +": NULL");
          res += 10000;
        } else {
          if(dbg_) Log.i(TAG, "idx:"+i +": " + a[i].getRes());
          res += a[i].getRes();
        }
      }
    }
    CheckTest.Run(name + ":" + info + "Obj", res + " (C:" + correctRes+")", (res == correctRes));
  }
}
