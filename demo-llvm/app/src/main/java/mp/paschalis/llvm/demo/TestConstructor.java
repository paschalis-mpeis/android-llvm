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

public class TestConstructor {
  boolean a;
  byte b;
  char c;
  short d;
  int e;
  long f;
  double g;
  float h;

  TestConstructor() {
    a = true;
    b = 4;
    c = 'a';
    d = 2;
    e = 6;
    f = 3;
    g = 9.51;
    h = 3.14f;

  }

  void VerifyC1() {
    Log.e(TAG, "TestConstructor:" + a + " " + (a == true ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + b + " " + (b == 4 ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + c + " " + (c == 'a' ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + d + " " + (d == 2 ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + f + " " + (f == 3 ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + g + " " + (g == 9.51 ? "CORRECT" : "FALSE"));
    Log.e(TAG, "TestConstructor:" + h + " " + (h == 3.14f ? "CORRECT" : "FALSE"));
  }
}
