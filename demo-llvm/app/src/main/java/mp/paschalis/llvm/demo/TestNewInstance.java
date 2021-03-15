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
import mp.paschalis.llvm.demo.TestInheritance.Car;

public class TestNewInstance {

  static void RunTests() {

    verifyT1(TestNewInstance.T1());
    verifyHumanDefault(TestNewInstance.T2());
    verifyHumanCustomCtor(TestNewInstance.T3());
    verifyNewArrayInt(NewArrayInt());
    verifyNewArrayByte(NewArrayByte());
    verifyNewArrayDouble(NewArrayDouble());
    verifyNewArrayObject(NewArrayObject());
    verifyNewArrayInt2D(NewArrayInt2D());
    verifyNewArrayLong2D(NewArrayLong2D());
    verifyNewArrayLong2D_returnArray(NewArrayLong2D_returnArray());
  }

  private static int T1() {
    Car c = new Car();
    return c.accelerate();
  }


  private static int T2() {
    Human h = new Human();
    return h.getRes();
  }

  private static int T3() {
    Human h = new Human(34, 43);
    return h.getRes();
  }

  private static int[] NewArrayInt() {
    int x[] = new int[10];
    for (int i = 0; i < x.length; i++) {
      x[i] = i + 123;
    }
    return x;
  }

  private static double[] NewArrayDouble() {
    double x[] = new double[20];
    for (int i = 0; i < x.length; i++) {
      x[i] = i + 34.86789545;
    }
    return x;
  }


  private static byte[] NewArrayByte() {
    byte x[] = new byte[50];
    for (int i = 0; i < x.length; i++) {
      x[i] = (byte) (i + 5);
    }
    return x;
  }


  private static int[][] NewArrayInt2D() {
    return new int[10][5];
  }

  private static long NewArrayLong2D() {
    long x[][] = new long[50][10];
    for (int i = 0; i < x.length; i++) {
      for (int j = 0; j < x[i].length; j++) {
        x[i][j] = 1345;
      }
    }
    int sum = 0;
    for (int i = 0; i < x.length; i++) {
      for (int j = 0; j < x[i].length; j++) {
        sum += x[i][j];
      }
    }

    return sum;
  }

    private static long[][] NewArrayLong2D_returnArray() {
    long x[][] = new long[50][10];
    for (int i = 0; i < x.length; i++) {
      for (int j = 0; j < x[i].length; j++) {
        x[i][j] = 1345;
      }
    }
    return x;
  }



  private static Human[] NewArrayObject() {
    Human x[] = new Human[20];
    for (int i = 0; i < x.length; i++) {
      x[i] = new Human(2, 6);
    }
    return x;
  }


  static void verifyNewArrayObject(Human[] x) {
    int res = 0;
    for (int i = 0; i < x.length; i++) {
      res += x[i].getRes();
    }
    CheckTest.Run("TestNewArrayObject: ", res + "", (res == 620));
  }


  static void verifyNewArrayInt2D(int[][] newarr) {

    CheckTest.Run("TestNewArrayInt2D: ", "len: X: " + newarr.length, (newarr.length == 10));
    CheckTest.Run("TestNewArrayInt2D: ", "len: X[0].length: " + newarr[0].length, (newarr[0].length == 5));
    for (int i = 0; i < newarr.length; i++) {
      for (int j = 0; j < newarr[i].length; j++) {
        newarr[i][j] = i;
      }
    }
    int sum = 0;
    for (int i = 0; i < newarr.length; i++) {
      for (int j = 0; j < newarr[i].length; j++) {
        sum += newarr[i][j];
      }
    }

    CheckTest.Run("TestNewArrayInt2D: ", "SUM: " + sum, (sum == 225));
  }


  static void verifyNewArrayInt(int[] x) {
    int res = 0;
    for (int i = 0; i < x.length; i++) {
      res += x[i];
    }
    CheckTest.Run("TestNewArrayInt: ", res + "", (res == 1275));
  }

  private static void verifyNewArrayLong2D(long res) {
    CheckTest.Run("TestNewArrayLong2D: ", res + "", (res == 672500));
  }

  private static void verifyNewArrayLong2D_returnArray(long x[][]) {
    long res = 0;
    for (int i = 0; i < x.length; i++) {
      for (int j = 0; j < x[i].length; j++) {
        res += x[i][j];
      }
    }
    CheckTest.Run("TestNewArrayLong2D_returnArray: ", res + "", (res == 672500));
  }

  static void verifyNewArrayDouble(double[] x) {
    double res = 0;
    for (int i = 0; i < x.length; i++) {
      res += x[i];
    }
    CheckTest.Run("TestNewArrayDouble: ", res + "", (res == 887.357909));
  }

  static void verifyNewArrayByte(byte[] x) {
    int res = 0;
    for (int i = 0; i < x.length; i++) {
      res += x[i];
    }
    CheckTest.Run("TestNewArrayByte: ", res + "", (res == 1475));
  }

  static void verifyT1(int res) {
    CheckTest.Run("TestNewInstance: T1", res + "", (res == 62));
  }


  static void verifyHumanDefault(int res) {
    CheckTest.Run("TestNewInstance: T2:Human(): ", res + "", (res == 23));
  }

  static void verifyHumanCustomCtor(int res) {
    CheckTest.Run("TestNewInstance: T3:Human(int, int): ", res + "", (res == 100));
  }
}
