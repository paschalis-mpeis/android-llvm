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

public class TestConditions {

  public static void RunTests() {
    TestConditions t = new TestConditions();
    t.verifyT1(t.T1(13,344));
    boolean rt2 = t.T2(43,34.14f);
    t.verifyT2(rt2);
    t.verifyT3(t.T3((short) 3,3334));
    t.verifyT4(t.T4(2342343, (short) 34));
    t.verifyT5(t.T5(23.567567, (byte) 3));
    t.verifyT6(t.T6(34.665f, 34));
  }

  boolean T1(int x, int z) {
    if (x < z) {
      return true;
    }
    return false;
  }

  public void verifyT1(boolean res) {
    CheckTest.Run("TestConditionsT1:", res + "", (res == true));
  }

  boolean T2(int x, float z) {
    if (x >= z) {
      return true;
    }
    return false;
  }

  public void verifyT2(boolean res) {
    CheckTest.Run("TestConditionsT2: res:", res + "", (res == true));
  }


  boolean T3(short x, long z) {
    if (x == z) {
      return true;
    }
    return false;
  }

  public void verifyT3(boolean res) {
    CheckTest.Run("TestConditionsT3", res + "", (res == false));
  }


  boolean T4(long x, short z) {
    if (x <= z) {
      return true;
    }
    return false;
  }

  public void verifyT4(boolean res) {
    CheckTest.Run("TestConditionsT4", res + "", (res == false));
  }

  boolean T5(double x, byte z) {
    if (x > z) {
      return true;
    }
    return false;
  }

  public void verifyT5(boolean res) {
    CheckTest.Run("TestConditionsT5", res + "", (res == true));
  }

  boolean T6(float x, double z) {
    if (x != z) {
      return true;
    }
    return false;
  }

  public void verifyT6(boolean res) {
    CheckTest.Run("TestConditionsT6", res + "", (res == true));
  }
}
