package mp.paschalis.llvm.demo;

/**
 * Created by paschalis on 06/07/2017.
 */
public class ArithmeticOps {

  public static void RunTests() {
    ArithmeticOps t = new ArithmeticOps();
    t.verifyArithmeticAdd(t.TestAdd(12342345, 123));
    t.verifyArithmeticSub(t.TestSub(34.34f, 34.3821f));
    t.verifyArithmeticMul(t.TestMul(53, 255));
    t.verifyArithmeticDiv(t.TestDiv(34.342252356, 56.56473366));
  }

  long TestAdd(long x, int y) {
    long r = 10;
    r += y * 2 * 3;
    r += x + x + x;
    return r;
  }

  public void verifyArithmeticAdd(long res) {
    CheckTest.Run("TestAdd", res + "", (res == 37027783));
  }

  double TestSub(float x, float y) {
    double r = 4.34244;
    r -= (x * 5);
    r -= y;
    return r;
  }

  public void verifyArithmeticSub(double res) {
    CheckTest.Run("Testsub", res + "", (res == -201.7396560998535));
  }

  int TestMul(int x, int y) {
    int r = 54;
    r += y * y;
    return r;
  }

  public void verifyArithmeticMul(int res) {
    CheckTest.Run("TestMul", res + "", (res == 65079));
  }

  double TestDiv(double x, double y) {
    double r = 3245.343;
    r /= x;
    r /= ((y + 10) * 2);
    return r;
  }

  public void verifyArithmeticDiv(double res) {
    CheckTest.Run("TestDiv", res + "", (res == 0.7098353680730339));
  }

}
