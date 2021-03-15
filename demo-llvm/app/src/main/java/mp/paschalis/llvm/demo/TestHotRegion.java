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

/**
 * Checks that we can enter the hot region regardless of the apps type.
*/
public class TestHotRegion {
    public static void RunTests() {
        int result = _HotRegionEntrypoint();
        verify("RESULT: ", result, 243);
    }

  /**
   * The outer method that will call the HFs.
   * This should be normally compiled (like with Quick, or interpreter, whatever is the default).
   *
   * I think this is no longer in use. I made this when I was trying to figure out how to
   * compile everything with ART, and still jump to RT for LLVM calls when I needed to.
   * (previously I was forcing all to interpretation so I could hijack runtime.
   * I can still do this now)
   */
  static int _HotRegionEntrypoint() {
        CParent cp = new CParent(3, 5);
        CParent cc = new CChild(3, 5);
        CParent.StaticMethod(1,6);
        int a = cp.DirectMethod(3,6);
        int b = cp.VirtualMethod(4,7);
        int c = cp.InterfaceMethod(2,6);
        int d = cc.VirtualMethod(4,7);
        int e = cc.InterfaceMethod(100,6);

        verify("cp.DirectMethod", a, 10);
        verify("cp.VirtualMethod", b, 4);
        verify("cp.InterfaceMethod", c, 99);
        verify("cc.VirtualMethod", d, 14);
        verify("cc.InterfaceMethod", e, 116);

        return a + b + c + d + e;
    }

    static void verify(String info, int res, int correct_res) {
        CheckTest.Run(info, res + "", (res == correct_res));
  }


    static int MathSqrt(int a, int b) {
        int result = (int) Math.sqrt(a + b);
        Log.i(TAG, "MathSqrt: " + result);
        return result;
    }


    public static interface CInterface {
        int InterfaceMethod(int a, int b);
    }

    public static class CParent implements CInterface {
        int a;
        int b;

        CParent(int a, int b) {
            this.a = a;
            this.b = b;
        }


        static int StaticMethod(int a, int b) {
            int result = Math.max(a, b);
            Log.i(TAG, "StaticMethod: " + result);
            return result;
        }

        final int DirectMethod(int a, int b) {
            int result = (int) Math.sqrt(a + b + 100);
            Log.i(TAG, "DirectMethod: " + result);
            return result;
        }


        int VirtualMethod(int a, int b) {
            int result = Math.min(a, b);
            Log.i(TAG, "VirtualMethod: " + result);
            return result;
        }


        @Override
        public int InterfaceMethod(int a, int b) {
            int result = (int) Math.max(a+b+10, 99);
            Log.i(TAG, "InterfaceMethod: " + result);
            return result;
        }
    }


    // CHECK direct
    // CHECK static
    // CHECK virtual
    // CHECK interface

    public static class CChild extends CParent {

        CChild(int a, int b) {
            super(a, b);
        }


        int VirtualMethod(int a, int b) {
            int result = Math.min(a + 10, b + 10);
            Log.i(TAG, "CChild: VirtualMethod: " + result);
            return result;
        }

    }

}
