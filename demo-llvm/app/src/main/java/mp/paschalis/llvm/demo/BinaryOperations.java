package mp.paschalis.llvm.demo;

/**
 * Created by paschalis on 06/06/2017.
 */
public class BinaryOperations {

  long s = 11;
  long t;

  // TODO 4.345/0  div w/ zero? float / int?

  public static void RunTests() {

    BinaryOperations t = new BinaryOperations();
    t.DoBinaryOperations();
    t.verifyBinaryOperations(t.s);

    short z = 3;
    t.argumentsTest(1000, 100, z);
    t.verifyArguments(t.t);

    t.verifyFRem(t.TestFRem(234.46f, 3.434f));
    t.verifyFRem2(t.TestFRem(3.434f, 234.46f));
    t.verifyFRem_double(t.TestFRem_double(453.43423425, 234.4623423344));
    t.verifyURem(t.TestURem('g', 'a'));
    t.verifySRem(t.TestSRem(7234, 2767));

    t.verifyAndT1(t.TestAnd(15, 6));
    t.verifyAndT2(t.TestAnd(15, 3));
    t.verifyAndT3(t.TestAnd(8, 6));
    t.verifyAndT4(t.TestAnd(8, 3));

    t.verifyOrT1(t.TestOr(10, 10));
    t.verifyOrT2(t.TestOr(55, 10));
    t.verifyOrT3(t.TestOr(10, 15));
    t.verifyOrT4(t.TestOr(55, 15));

    // TODO rest of neg ..
    t.verifyNegInt(t.TestNegInt(34567));
    t.verifyNegByte(t.TestNegByte((byte) 4));
    t.verifyNegChar(t.TestNegChar('a'));
    t.verifyNegFloat(t.TestNegFloat(5.3455f));
    t.verifyNegDouble(t.TestNegDouble(534.312323455));
    t.verifyNegShort(t.TestNegShort((short) 159));


    t.verifyXorI1(t.TestXorI(2323, 34));
    t.verifyXorI2(t.TestXorI(-324, -456));
    t.verifyXorI3(t.TestXorI(324, -456));
    t.verifyXorL(t.TestXorL(2121233232343L, 3243434));
  }


  public boolean TestAnd(int a, int b) {
    if (a > 10 && b > 5) {
      return true;
    }
    return false;
  }

  public boolean TestOr(int a, int b) {
    if (a < 50 || b < 11) {
      return true;
    }
    return false;
  }


  public void verifyAndT1(boolean res) {
    CheckTest.Run("BinOps: And:T1: ", res + "", (res == true));
  }

  public void verifyAndT2(boolean res) {
    CheckTest.Run("BinOps: And:T2: ", res + "", (res == false));
  }

  public void verifyAndT3(boolean res) {
    CheckTest.Run("BinOps: And:T3: ", res + "", (res == false));
  }

  public void verifyAndT4(boolean res) {
    CheckTest.Run("BinOps: And:T4: ", res + "", (res == false));
  }

  public void verifyOrT1(boolean res) {
    CheckTest.Run("BinOps: Or:T1: ", res + "", (res == true));
  }

  public void verifyOrT2(boolean res) {
    CheckTest.Run("BinOps: Or:T2: ", res + "", (res == true));
  }

  public void verifyOrT3(boolean res) {
    CheckTest.Run("BinOps: Or:T3: ", res + "", (res == true));
  }

  public void verifyOrT4(boolean res) {
    CheckTest.Run("BinOps: Or:T4: ", res + "", (res == false));
  }

  public int TestXorI(int a, int b) {
    return a ^ b;
  }

  public long TestXorL(long a, long b) {
    return a ^ b;
  }

  public void verifyXorI1(int res) {
    CheckTest.Run("BinOps: XorI:1: ", res + "", (res == 2353));
  }

  public void verifyXorI2(int res) {
    CheckTest.Run("BinOps: XorI:2: ", res + "", (res == 132));
  }

  public void verifyXorI3(int res) {
    CheckTest.Run("BinOps: XorI:3: ", res + "", (res == -132));
  }

  public void verifyXorL(long res) {
    CheckTest.Run("BinOps: XorL: ", res + "", (res == 2121234320509L));
  }

  public float TestFRem(float a, float b) {
    return a % b;
  }


  public void verifyFRem(float res) {
    CheckTest.Run("BinOps: FRem: ", res + "", (res == 0.9480057f));
  }

  public void verifyFRem2(float res) {
    CheckTest.Run("BinOps: FRem2: ", res + "", (res == 3.434f));
  }

  public double TestFRem_double(double a, double b) {
    return a % b;
  }

  public void verifyFRem_double(double res) {
    CheckTest.Run("BinOps: FRem_double: ", res + "", (res == 218.97189191559997));
  }

  public int TestURem(char a, char b) {
    return a % b;
  }


  public int TestNegInt(int a) {
    return -a;
    // TODO tests ]remaining types?
    // long, short, char, byte,
  }

  public short TestNegShort(short a) {
    return (short) -a;
  }

  public float TestNegFloat(float a) {
    return -a;
  }

  public double TestNegDouble(double a) {
    return -a;
  }


  public byte TestNegByte(byte a) {
    return (byte) -a;
  }



  public char TestNegChar(char a) {
    return (char) -a;
  }

  public void verifyNegInt(int res) {
    CheckTest.Run("BinOps: NegInt: ", res + "", (res == -34567));
  }

  public void verifyNegShort(short res) {
    CheckTest.Run("BinOps: NegShort: ", res + "", (res == (short) -159));
  }

  public void verifyNegFloat(float res) {
    CheckTest.Run("BinOps: NegFloat: ", res + "", (res == -5.3455f));
  }

  public void verifyNegDouble(double res) {
    CheckTest.Run("BinOps: NegDouble: ", res + "", (res == -534.312323455));
  }

  public void verifyNegByte(byte res) {
    CheckTest.Run("BinOps: NegByte: ", res + "", (res == -4));
  }

  public void verifyNegChar(char res) {
    CheckTest.Run("BinOps: NegChar: ", res + "/" + (int) res, (res == 65439));
  }


  public void verifyURem(int res) {
    CheckTest.Run("BinOps: URem: ", res + "", (res == 6));
  }

  public int TestSRem(int a, int b) {
    return a % b;
  }

  public void verifySRem(int res) {
    CheckTest.Run("BinOps: SRem: ", res + "", (res == 1700));
  }

  public void DoBinaryOperations() {
    s -= 1;
    for (int i = 0; i < 1000000; i++) {
      s += ((i + 1) * 3 / 4);
    }
    // Outcome: 375000000010
  }

  public void argumentsTest(long x, int y, short p) {
    t = x / 2;
    for (int i = 0; i < y; i++) {
      t += p;
    }
    // Outcome: 800
  }

  public void verifyBinaryOperations(long res) {
    CheckTest.Run("BinOps: TestBinaryOperations: ", res + "", (res == 375000000010L));
  }

  public void verifyArguments(long res) {
    CheckTest.Run("BinOps: TestArguments: ", res + "", (res == 800));
  }

}
