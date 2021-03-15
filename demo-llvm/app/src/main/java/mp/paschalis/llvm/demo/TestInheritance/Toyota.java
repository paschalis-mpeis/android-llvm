package mp.paschalis.llvm.demo.TestInheritance;

import mp.paschalis.llvm.demo.CheckTest;

/**
 * Created by paschalis on 31/07/2017.
 */

public class Toyota extends Car {


  static void verifyAccelerate(int res) {
    CheckTest.Run("TestInheritance: Toyota: accelerate: ", res + "", (res == 73));
  }

  static void verifyTopSpeed(int res) {
    CheckTest.Run("TestInheritance: Toyota: topSpeed: ", res + "", (res == 260));
  }

  public int accelerate() {
    return super.accelerate() + 11;
  }

  public int topSpeed() {
    return 180;
  }

}
