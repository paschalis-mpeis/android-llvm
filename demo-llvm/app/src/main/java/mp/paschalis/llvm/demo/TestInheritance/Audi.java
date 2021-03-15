package mp.paschalis.llvm.demo.TestInheritance;

import mp.paschalis.llvm.demo.CheckTest;

/**
 * Created by paschalis on 31/07/2017.
 */

public class Audi extends Car {

  public static void verifyCallInterface(int m) {
    CheckTest.Run("TestCallInterface: Value: ", m + "", (m== 14035));
  }

  @Override
  public int MusicLevel(int input) {
    int s=10;
    for(int i=0; i<input; i++) {
      s+=this.steerLeft()*4+i;
    }
    return s;
  }

  static void verifyInterface(int m) {
    CheckTest.Run("TestInterface: Audi: music lvl: ", m + "", (m== 1136));
  }

  static void verifyAccelerate(int res) {
    CheckTest.Run("TestInheritance: Audi: accelerate: ", res + "", (res == 6076));
  }

  static void verifyTopSpeed(int res) {
    CheckTest.Run("TestInheritance: Audi: topSpeed: ", res + "", (res == 1606));
  }

  static void verifyDebug1(int res) {
    CheckTest.Run("TestInheritance: Audi: Debug1: ", res + "", (res == 5478));
  }

  static void verifyDebug2(int res) {
    CheckTest.Run("TestInheritance: Audi: Debug2: ", res + "", (res == 6033));
  }

  static void verifyDebug3(int res) {
    CheckTest.Run("TestInheritance: Audi: Debug3: ", res + "", (res == 6074));
  }

  static void verifyDebug4(int res) {
    CheckTest.Run("TestInheritance: Audi: Debug4: ", res + "", (res == 6074));
  }

  public int TestAccelerate() {
    // if it's called by an audi object it will call the accelerate it expects (Audi's)
    return accelerate();
  }

  public int Debug1() {  // VERIFIED
    int res = 19;
    for (int i = 0; i < 100; i++) {
      res += 4;
    }
    res += CarDirectMethod__NOT_HOT__();
    return res;
  }

  public int Debug2() {  // VERIFIED
    int res = 19;
    for (int i = 0; i < 100; i++) {
      res += 4;
    }

    return res + CarDirectMethod__NOT_HOT__() + CarStaticMethod__NOT_HOT__();
  }

  public int Debug3() {  //  VERIFIED
    // This is an exact copy of accelerate
    int res = super.accelerate();
    for (int i = 0; i < 100; i++) {
      res += 4;
    }
    return res + CarDirectMethod__NOT_HOT__() + CarStaticMethod__NOT_HOT__();
  }


  public int Debug4() {  // VERIFY
    return Debug3();
  }

    static void verifyDebugTopSpeed(int res) {
    CheckTest.Run("TestInheritance: Audi: DebugTestTopSpeed: ", res + "", (res == 2816));
  }

  public int DebugTestTopSpeed() {
    return DebugTopSpeed();
  }

    public int DebugTopSpeed() {
    return super.DebugTopSpeed() + 50;
  }

  public int TestTopSpeed() {
    // if it's called by an audi object it will call the topSpeed it expects (Audi's)
    return topSpeed();
  }

  public int accelerate() {
    int res = super.accelerate();
    for (int i = 0; i < 100; i++) {
      res += 4;
    }
    return res + CarDirectMethod__NOT_HOT__() + CarStaticMethod__NOT_HOT__();
  }

  public int topSpeed() {
    return super.topSpeed() + 50;
  }

  @Override
  public int start() {
    return super.start() + 5;
  }

  @Override
  public int stop() {
    return super.stop() + 5;
  }

  public int CallInterface(int i) {
    return MusicLevel(i+10) + super.MusicLevel(5*i);
  }
}

