package mp.paschalis.llvm.demo.TestInheritance;

import mp.paschalis.llvm.demo.CheckTest;

/**
 * Created by paschalis on 31/07/2017.
 *
 * This is not hot.
 */

public final class Nissan extends Car {

  static void verifyAccelerate(int res) {
    CheckTest.Run("TestInheritance: Nissan: accelerate: ", res + "", (res == 95));
  }

  static void verifyTopSpeed(int res) {
    CheckTest.Run("TestInheritance: Nissan: topSpeed: ", res + "", (res == 270));
  }

  public int accelerate() {
    return super.accelerate() + 33;
  }

  public int topSpeed() {
    return 190;
  }

}
