package mp.paschalis.llvm.demo.TestInheritance;

import mp.paschalis.llvm.demo.CheckTest;

/**
 * Created by paschalis on 31/07/2017.
 */

public class Car extends Vehicle implements  Infortaintment{

  public int testInt = 1;

  /**
   * NOTE:
   *
   * Things testing:
   * <p>
   * <p>
   * Car and Audi are hot.
   * Toyota is not.
   * So when calling Toyota stuff, should go through RT.
   * <p>
   * Car is hot
   * <p>
   * Audi: it's store in an Audi object, so when calling
   * accelerate and topSpeed we should be calling what we
   * would expect.
   * <p>
   * <p>
   * Toyota: we store a Toyota object in a Car handle.
   * So when calling accelerate, the HGraph would initially
   * expect a Car.accelerate, however ResolveVirtualMethod
   * correctly returns Toyota.accelerate.
   * <p>
   * In this case it have to go through RT.
   * TODO Case a: It's hot so calling again llvm code
   * (in detail: llvm code -> calls RT -> calls LLVM Code)
   * <p>
   * TODO Case b: it's not hot: RT will take care of executing it!
   */
  public static void TestInheritance() {
    // TODO abstract?

    Audi audi = new Audi();
    Car t = new Toyota();
    Car c = new Car();
    Nissan n = new Nissan();

    Audi.verifyAccelerate(audi.TestAccelerate());
    Audi.verifyTopSpeed(audi.TestTopSpeed());
    // we call this twice so the invoke.hist will make audi methods preferable
    Audi.verifyTopSpeed(audi.TestTopSpeed());
    // Audi.verifyDebugTopSpeed(audi.DebugTestTopSpeed());
    // DebugAudi da = new DebugAudi();
    // DebugAudi.verifyDebug4(da.Debug4());
    // Audi.verifyDebug1(audi.Debug1());
    // Audi.verifyDebug2(audi.Debug2());
    // Audi.verifyDebug3(audi.Debug3());
    // Audi.verifyDebug4(audi.Debug4());

    Toyota.verifyAccelerate(t.TestAccelerate());
    Toyota.verifyTopSpeed(t.TestTopSpeed());

    Nissan.verifyAccelerate(n.TestAccelerate());
    Nissan.verifyTopSpeed(n.TestTopSpeed());

    Car.verifyAccelerate(c.TestAccelerate());
    Car.verifyTopSpeed(c.TestTopSpeed());

    Car.verifyInterface(c.MusicLevel(10));
    Audi.verifyInterface(audi.MusicLevel(4));

    Audi.verifyCallInterface(audi.CallInterface(10));

  }

  private static void verifyInterface(int m) {
    CheckTest.Run("TestInterface: Car: music lvl: ", m + "", (m== 1455));
  }

  static void verifyAccelerate(int res) {
    CheckTest.Run("TestInheritance: Car: accelerate: ", res + "", (res == 62));
  }

  static void verifyTopSpeed(int res) {
    CheckTest.Run("TestInheritance: Car: topSpeed: ", res + "", (res == 1636));
  }

  public static int CarStaticMethod__NOT_HOT__() {
    int sum = 3;
    for (int i = 0; i < 45; i++) {
      if (i % 2 == 0) {
        sum += i + 2;
      }
    }
    return sum;
  }

  public int TestAccelerate() {
    // Unless we call this w/ a Car object, this will have to go through RT
    return accelerate();
  }


  public int TestTopSpeed() {
    // Unless we call this w/ a Car object, this will have to go through RT
    return topSpeed() + steerLeft() + steerRight();
  }

  public int accelerate() {
    int res = 10;
    testInt++;
    for (int i = 0; i < 100; i++) {
      if (i % 2 == 0) {
        res++;
      }

    }
    return res + testInt;
  }

  public final int CarDirectMethod__NOT_HOT__() {
    int sum = 10;
    for (int i = 0; i < 100; i++) {
      if (i % 3 == 0) {
        sum += i * 3;
      }
    }
    return sum;
  }


  @Override
  public int start() {
    int sum = 1;
    for (int i = 0; i < 45; i++) {
      if (i % 4 == 0) {
        sum += i * 10;
      }
    }
    return sum;
  }

  @Override
  public int stop() {
    int sum = 5;
    for (int i = 0; i < 30; i++) {
      if (i % 8 == 0) {
        sum += i * 25;
      }
    }
    return sum;
  }

  @Override
  public int steerLeft() {
    int sum = 10;
    for (int i = 0; i < 10; i++) {
      if (i % 2 == 0) {
        sum += i * 3;
      }
    }
    return sum;
  }

  @Override
  public int steerRight() {
    int sum = 100;
    for (int i = 0; i < 5; i++) {
      if (i % 2 == 0) {
        sum -= 15 * i;
      }
    }
    return sum;
  }

  @Override
  public int MusicLevel(int input) {
    int s=10;
    for(int i=0; i<input; i++) {
     s+=this.steerLeft()*2+i;
    }
    return s;
  }
}
