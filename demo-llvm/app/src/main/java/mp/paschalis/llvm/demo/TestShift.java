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

public class TestShift {

  static void RunTests() {
    TestShift t = new TestShift();
    t.verifyTestShl1(t.TestShl1(12345));
    t.verifyTestShl2(t.TestShl2(28, 7));
    t.verifyTestShr1(t.TestShr1(7234));
    t.verifyTestShr2(t.TestShr2(4343443, 3));
    t.verifyTestUShr1(t.TestUShr1('a'));
  }

  long TestShl1(long i) {
    return i << 4;
  }

  void verifyTestShl1(long res) {
    CheckTest.Run("TestShl1", res + "", (res == 197520));
  }

  long TestShl2(long i, int shl) {
    return i << shl;
  }

  void verifyTestShl2(long res) {
    CheckTest.Run("TestShl2", res + "", (res == 3584));
  }


  int TestShr1(int i) {
    return i >> 2;
  }

  void verifyTestShr1(int res) {
    CheckTest.Run("TestShr1", res + "", (res == 1808));
  }


  int TestShr2(int i, int shr) {
    return i >> shr;
  }

  void verifyTestShr2(int res) {
    CheckTest.Run("TestShr2", res + "", (res == 542930));
  }

  char TestUShr1(char a) {
    return (char) (a >>> 2);
  }

  void verifyTestUShr1(char res) {
    CheckTest.Run("TestUShr1", res + " as int(" + (int) res + ")", (((int) res) == 24));
  }
}
