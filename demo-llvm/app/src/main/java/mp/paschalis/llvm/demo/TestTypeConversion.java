/*
 * Copyright 2018 Paschalis Mpeis
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

/**
 * This has getters and setters, flaoting point, castings, ifs
 */

public class TestTypeConversion {
  double a = 11.235;
  double b = 9.3456;
  float c = 3.45f;


  static void RunTests() {
    TestTypeConversion t = new TestTypeConversion();

    t.verifyT("T1", t.T1(5), 5.0f);
    t.verifyT("T2", t.T2(34348), 34348.0);
    t.verifyT("T3", t.T3(6.3246f), 6);
    t.verifyT("T4", t.T4(543243.2342341f), 543243);
    t.verifyT("T5", t.T5(5.2342423f), 5.2342424392700195);
    t.verifyT("T6", t.T6(523423423.2342342343224), 5.23423424E8);
    t.verifyT("T7", t.T7(825823853855L), 1190133023);
    t.verifyT("T8", t.T8(534324), 534324);
  }

  float T1(int x) {
    return x;
  }

  double T2(long x) {
    return x;
  }


  int T3(float x) {
    return (int) x;
  }

  long T4(double x) {
    return (long) x;
  }

  double T5(float x) {
    return (double) x;
  }

  float T6(double x) {
    return (float) x;
  }

  int T7(long x) {
    return (int) x;
  }

  long T8(int  x) {
    return (long ) x;
  }

  public void verifyT(String info, float r, float c) {
    CheckTest.Run(TestTypeConversion.class.getSimpleName() +" " + info,
      r + "", (r == c));
  }

  public void verifyT(String info, double r, double c) {
    CheckTest.Run(TestTypeConversion.class.getSimpleName() +" " + info,
      r + "", (r == c));
  }
  public void verifyT(String info, int r, int c) {
    CheckTest.Run(TestTypeConversion.class.getSimpleName() +" " + info,
      r + "", (r == c));
  }

  public void verifyT(String info, long r, long c) {
    CheckTest.Run(TestTypeConversion.class.getSimpleName() +" " + info,
      r + "", (r == c));
  }
}
