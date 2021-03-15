package mp.paschalis.llvm.demo.TestInheritance;

import mp.paschalis.llvm.demo.CheckTest;

/**
 * Created by paschalis on 31/07/2017.
 */

public class DebugAudi extends Car {

  static void verifyDebug4(int res) {
    CheckTest.Run("TestInheritance: DebugAudi: Debug4: ", res + "", (res == 6074));
  }

  public int Debug3() {  //  VERIFIED
    // This is an exact copy of accelerate
    int res = super.accelerate();
    for (int i = 0; i < 100; i++) {
      res += 4;
    }
    return res + CarDirectMethod__NOT_HOT__() + CarStaticMethod__NOT_HOT__();
  }


  public int Debug4() {  // FAILS
    return Debug3();
  }
}

