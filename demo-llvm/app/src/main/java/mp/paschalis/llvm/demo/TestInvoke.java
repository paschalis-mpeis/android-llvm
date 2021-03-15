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

import mp.paschalis.llvm.demo.TestInheritance.Car;

import static mp.paschalis.llvm.demo.Debug.TAG;

public class TestInvoke {

  static void RunTests() {

    verifyStaticA(StaticA());

    TestInvoke t = new TestInvoke();
    verifyDirectA(t.DirectA());
    verifyDirectAA(t.DirectAA());

    verifyVirtualA(t.VirtualA(23));

    verifyInvokeJavaMethod(InvokeJavaMethod());
    verifyVirtualDebugA(t.VirtualDebugA(3));
    Car.TestInheritance();
  }


  private static int InvokeJavaMethod() {
    Runtime r = Runtime.getRuntime();
    Log.i(TAG, "MEM: " + r.maxMemory());
    return 10;
  }

  static void verifyInvokeJavaMethod(int res) {
    CheckTest.Run("TestInvoke: InvokeJavaMethod", res + "", (res == 10));
  }

  private static int StaticA() {
    int i = 10;
    return StaticB(i) + StaticD(i + 1000);
  }

  static int StaticB(int i) {
    return ++i + StaticC(i + 10);
  }

  static int StaticC(int in) {
    int sum = in;
    for (int i = 0; i < in; i++) {
      sum += i * 2 + 1;
    }
    return sum;
  }

  static int StaticD(int in) {
    return StaticB(in + 28);
  }

  static void verifyStaticA(int res) {
    CheckTest.Run("TestInvoke: StaticA", res + "", (res == 1102962));
  }

  static void verifyDirectAA(int res) {
    CheckTest.Run("TestInvoke: DirectAA", res + "", (res == 1102962));
  }

  static void verifyDirectA(int res) {
    CheckTest.Run("TestInvoke: DirectA", res + "", (res == 924565508));
  }

  static void verifyVirtualA(long res) {
    CheckTest.Run("TestInvoke: VirtualA", res + "", (res == 1204878));
  }

  static void verifyVirtualDebugA(long res) {
    CheckTest.Run("TestInvoke: VirtualDebugA", res + "", (res == 15));
  }

  private int DirectAA() {
    int i = 10;
    return DirectBB(i) + DirectDD(i + 1000);
  }

  final int DirectBB(int i) {
    return ++i + DirectCC(i + 10);
  }

  final int DirectCC(int in) {
    int sum = in;
    for (int i = 0; i < in; i++) {
      sum += i * 2 + 1;
    }
    return sum;
  }

  final int DirectDD(int in) {
    return DirectBB(in + 28);
  }

  int DirectA() {
    int sum = 0;
    for (int i = 0; i < 100; i++) {
      sum += i + 2;
    }
    sum += DirectC(sum);
    return DirectB(sum);
  }

  final int DirectB(int sum) {
    for (int i = 3; i < 23; i++) {
      sum += i + 2;
    }
    return sum;
  }

  final int DirectC(int in) {
    int res = 0;
    for (int i = 0; i < in; i++) {
      res += i + 4;
    }
    return DirectD(res);
  }

  final int DirectD(int in) {
    int res = 0;
    for (int i = 0; i < in + 10; i++) {
      res += i * 2;
    }
    return res;
  }

  int VirtualDebugA(int in) {
    int sum = 0;
    for (int i = 0; i < in; i++) {
      int t = VirtualDebugB(in);
      Log.i(TAG, "itera:" + i + ":" + t);
      sum += t;
    }
    return sum;
  }

  int VirtualDebugB(int in) {
    int res = 0;
    res = 1 + res + in;
    res++;
    Log.i(TAG, "b: " + res);
    return res;
  }


  long VirtualA(long in) {
    long sum = in;
    for (int i = 0; i < in; i++) {
      sum += VirtualB(i);
    }
    return sum;
  }

  long VirtualB(long in) {
    in += 3;
    long res = in;
    for (int i = 0; i < in; i++) {
      res += i + 10;
    }

    return res + VirtualC(res);
  }

  long VirtualC(long in) {
    long sum = in + 5;
    for (int i = 0; i < in; i++) {
      sum += i + 11;
    }
    return sum;
  }


}
