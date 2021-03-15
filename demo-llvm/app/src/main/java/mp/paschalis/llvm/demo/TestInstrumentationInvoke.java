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
import static mp.paschalis.llvm.demo.Debug.TAG;

public class TestInstrumentationInvoke {

  static void RunTests() {

    Animal a = new Cat();

    int r = a.methodA(14);
    CheckTest.Run("TestInstrumentationInvoke: ", r + "", (r == 6402));
  }

  static class Animal {
    int methodA(int a) {
      int r = methodB(a, a + 5) ;
      r +=methodC(r);
      r += methodD(r);
      return r;
    }

    int methodB(int a, int b) {
      return a*b;
    }

    int methodC(int a) {
      return a + methodD(5);
    }

    int methodD(int b) {
      Log.d(TAG, "Animal.methodD");
      return b * 2;
    }
  }

  static class Cat extends Animal {
    int methodD(int b) {
      Log.d(TAG, "Cat.methodD");
      return b * 10;
    }
  }


}
