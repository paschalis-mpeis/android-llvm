/*
 * Copyright 2017 Paschalis Mpeis
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

public class TestUnaryOps {

  public static void RunTests() {
    TestUnaryOps t = new TestUnaryOps();

    t.verifyNopInt(t.NopInt(34));

    t.verifyNopLong(t.NopLong(234346643561L));
  }


  // TODO also long
  // TODO check that is only long or int!
  int NopInt(int a) {
    return ~a;
  }

  long NopLong(long a) {
    return ~a;
  }

  // TODO Xor


  public void verifyNopInt(int res) {
    CheckTest.Run("Test: NopInt", res + "", (res == -35));
  }

    public void verifyNopLong(long res) {
    CheckTest.Run("Test: NopLong", res + "", (res == -234346643562L));
  }


}
