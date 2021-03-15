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

public class TestVectorOps {

public static final int TestByte_testDotProdSimple(byte[] a, byte[] b) {
    int s = 1;
    for (int i = 0; i < b.length; i++) {
      int temp = a[i] * b[i];
      s += temp;
    }
    return s - 1;
  }

  static char[] a;
  static void TestCharSub(int x) {
    for (int i = 0; i < 128; i++)
      a[i] -= x;
  }

  static void TestCharNeg() {
    for (int i = 0; i < 128; i++)
      a[i] = (char) -a[i];
  }

  static void TestCharMul(int x) {
    for (int i = 0; i < 128; i++)
      a[i] *= x;
  }

  public static final int TestByte_testDotProdComplexUnsigned(byte[] a, byte[] b) {
    int s = 1;
    for (int i = 0; i < b.length; i++) {
      int temp = (((a[i] & 0xff) + 1) & 0xff) * (((b[i] & 0xff) + 1) & 0xff);
      s += temp;
    }
    return s - 1;
  }

  static void staticallyAligned(int[] a) {
    // Starts at offset 12 (hidden) + 1 * 4 relative to base alignment.
    // So no peeling, aligned vector, no cleanup.
    for (int i = 1; i < 9; i++) {
      a[i] += 1;
    }
  }
}
