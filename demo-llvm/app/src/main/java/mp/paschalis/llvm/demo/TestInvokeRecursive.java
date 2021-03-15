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

public class TestInvokeRecursive {

  static void RunTests() {
    TestInvokeRecursive t = new TestInvokeRecursive();

    verifyStaticFibonacci(StaticFibonacci(10));
    verifyDirectFibonacci(t.DirectFibonacci(10));
    verifyVirtualFibonacci(t.VirtualFibonacci(8));

    verifyWrapperStaticFibonacci(WrapperStaticFibonacci(8));
    verifyWrapperDirectFibonacci(t.WrapperDirectFibonacci(8));

    verifyWrapperStaticFibonacci(WrapperStaticFibonacci_callingNotHot(8));
    verifyWrapperDirectFibonacci(t.WrapperDirectFibonacci_callingNotHot(8));

    verifyWrapperVirtualFibonacci(t.WrapperVirtualFibonacci(8));

    verifyWrapperVirtualFibonacci(t.WrapperVirtualFibonacci_callingNotHot(8));
  }

  public static int WrapperStaticFibonacci_callingNotHot(int number) {
    return StaticFibonacci__NOT_HOT__(number);
  }


  public static int WrapperStaticFibonacci(int number) {
    return StaticFibonacci(number);
  }


  public static int StaticFibonacci(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return StaticFibonacci(number - 1) + StaticFibonacci(number - 2);
    // tail recursion
  }

  public static int StaticFibonacci__NOT_HOT__(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return StaticFibonacci__NOT_HOT__(number - 1) + StaticFibonacci__NOT_HOT__(number - 2);
    // tail recursion
  }


  static void verifyWrapperStaticFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: WrapperStaticFibonacci:", res + "", (res == 21));
  }

  static void verifyStaticFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: StaticFibonacci:", res + "", (res == 55));
  }


  static void verifyWrapperVirtualFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: WrapperVirtualFibonacci:", res + "", (res == 21));
  }


  static void verifyWrapperDirectFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: WrapperDirectFibonacci:", res + "", (res == 21));
  }

  static void verifyDirectFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: DirectFibonacci:", res + "", (res == 55));
  }


  static void verifyVirtualFibonacci(int res) {
    CheckTest.Run("TestInvokeRecursive: VirtualFibonacci:", res + "", (res == 21));
  }


  public final int WrapperDirectFibonacci(int number) {
    return DirectFibonacci(number);
  }

  public final int WrapperDirectFibonacci_callingNotHot(int number) {
    return DirectFibonacci__NOT_HOT__(number);
  }

  public final int DirectFibonacci(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return DirectFibonacci(number - 1) + DirectFibonacci(number - 2);
    // tail recursion
  }


  public final int WrapperVirtualFibonacci(int number) {
    return VirtualFibonacci(number);
  }

  public final int WrapperVirtualFibonacci_callingNotHot(int number) {
    return VirtualFibonacci__NOT_HOT__(number);
  }


  public int VirtualFibonacci__NOT_HOT__(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return VirtualFibonacci__NOT_HOT__(number - 1) + VirtualFibonacci__NOT_HOT__(number - 2);
    // tail recursion
  }

  public int VirtualFibonacci(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return VirtualFibonacci(number - 1) + VirtualFibonacci(number - 2);
    // tail recursion
  }


  public final int DirectFibonacci__NOT_HOT__(int number) {
    if (number == 1 || number == 2) {
      return 1;
    }

    return DirectFibonacci__NOT_HOT__(number - 1) + DirectFibonacci__NOT_HOT__(number - 2);
    // tail recursion
  }

}
