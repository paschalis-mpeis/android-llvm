package mp.paschalis.llvm.demo.TestInheritance;

/**
 * Created by paschalis on 10/08/2017.
 */

public abstract class Vehicle implements Engine, Wheel {



  public int topSpeed() {
    return 120 + start() - stop();
  }

  public int DebugTopSpeed() {
    return 120 + start();
  }
}
