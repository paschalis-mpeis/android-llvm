package mp.paschalis.llvm.demo;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;

import static mp.paschalis.llvm.demo.Debug.TAG;

public class RunTests {

  static final boolean VERIFIED_ON_ANDROID10 = false;
  private static final boolean TODO_VOLATILE = false;

  Context ctx;

  RunTests(Context ctx) {
    this.ctx=ctx;
  }

  public void DemoTests() {
  TestLoops.RunDemo();
  }

  public void CaptureTests() {
    TestReturnType.RunCaptureTest();
  }

  public void DebuggingTests() {
    TestInternalMethods.RunTestsDebug();
  }
  public void OsMethodsTests() {
    TestInvoke.RunTests();
    TestInvokeRecursive.RunTests();
    TestInstrumentationInvoke.RunTests();

    // can we enter the hot region? yes
    TestHotRegion.RunTests();
    // VERIFIED
    TestJni.RunTests();

    TestInternalMethods.RunTestsFull();
  }

  public void ArgsAndReturn() { // VERIFIED O0
    TestArguments.RunTests();
    TestReturnType.RunTests();
  }

  public void Basic1() {
    ArithmeticOps.RunTests();
    TestCasting.RunTests();
    TestUnaryOps.RunTests();
    TestShift.RunTests();
    TestFloatingPoint.RunTests();
    TestTypeConversion.RunTests();
  }

  public void Basic2() {
    TestConditions.RunTests();
    BinaryOperations.RunTests();
    TestLoops.RunTests();

    TestPackedSwitch.Run();
  }

  public void GettersSetters() {
    // Getters /Setters
    TestGetters.RunTests();
    TestSetters.RunTests();
    TestGettersSetters.RunTests();

    // before TestGettersStatic: we init the class in RT
    TestGettersStatic_FromOtherClass.RunTests();
    TestGettersStatic.RunTests();

    // before TestSettersStatic: we init the class in RT
    TestSettersStatic.RunTests();
    TestSettersStatic_fromOtherClass.RunTests();

    // Volatile Getters/Setters
    // Volatiles weren't implemented on android6
    if(TODO_VOLATILE) {
      TestGettersVolatile.RunTests();  // TODO implement all
      TestSettersVolatile.RunTests();  // TODO object, double, float
    }
  }

  public void CheckDivZero(int n) {
    TestDivNullCheck.RunTest(n);
  }

  public void CheckNull() {
    TestNullCheck.RunTest();
  }
  public void ChecksOutOfBounds(int n) {
    TestOutOfBounds.RunTest(n) ;
  }
  public void CheckInstanceOf() {
    TestInstanceOf.RunTests();
  }

  public void Strings() {
    TestString.RunTests();
  }

  public void InvokeQuickBasic() {
    TestLLVMtoQuick.RunTestsBasic();
  }
  public void InvokeQuickNested() {
    TestLLVMtoQuick.RunTestsNested();
  }

  public void InvokeQuickStaticBasic() {
    TestLLVMtoQuickStatic.RunTestsBasic();
  }

  public void InvokeQuickStaticNested() {
    TestLLVMtoQuickStatic.RunTestsNested();
  }

  public void Intrinsics() {
    // TODO charAt
    TestIntrinsics.RunTests();
  }

  public void Arrays() {
    TestArrays.RunTests();
    TestArrayGetSet.RunTests();
  }

  public void Vectors() {
    TestVecByte.main(null);
    TestVecChar.main(null);
    TestVecShort.main(null);
    TestVecInt.main(null);
    TestVecLong.main(null);
    TestVecFloat.main(null);
    TestVecDouble.main(null);
  }

  public void Exceptions() {
    TestExceptions.RunTests(); // TODO
  }

  public void TryCatch() { // TODO
    TestTryCatch.RunTests();
  }

  public void NewInstance() {
    // TODO implement: LoadClass: kBootImageRelRo
    TestNewInstance.RunTests();
  }

  private String res(int resource) {
    return ctx.getString(resource);
  }

  public static boolean Match(Context ctx, String val, int resource) {
    return (val.equals(ctx.getString(resource)));
  }

  private boolean Match(String val, int resource) {
    return Match(ctx, val, resource);
  }

  void RunSingle(String testUnit) {
    CheckTest.Reset();
    runTest(testUnit);
    CheckTest.ShowReport();
  }

  boolean IgnoreEntry(String unitTest, Context ctx) {
    String testAllEntry =  ctx.getString(R.string.TEST_ALL);
    return testAllEntry.equals(unitTest) || (unitTest.contains("[dies]"));
  }

  void RunAll() {
    CheckTest.Reset();
    Toast.makeText(ctx, "Running all!", Toast.LENGTH_SHORT).show();
    String[] testEntries = ctx.getResources().getStringArray(R.array.unit_test_labels);
    for (String unitTest: testEntries) {
      if (!IgnoreEntry(unitTest, ctx)) {
        Log.d(TAG, "TEST: " + unitTest);
        runTest(unitTest);
      }
    }
    Toast.makeText(ctx, "Tests: DONE!", Toast.LENGTH_SHORT).show();
    CheckTest.ShowReport();
  }

  private void runTest(String value) {
    if (Match(value, R.string.DEMO)) {
      Log.i(TAG, "DEMO MODE:");
      DemoTests();
    } else if (Match(value, R.string.capture_replay)) {
      CaptureTests();
      Log.i(TAG, "After capture..");
    } else if (Match(value, R.string.debugging)) {
      DebuggingTests();
    } else if (Match(value, R.string.os_methods)) {
      OsMethodsTests();
    } else if (Match(value, R.string.test_args_return)) {
      ArgsAndReturn();
    } else if (Match(value, R.string.test_basic1)) {
      Basic1();
    } else if (Match(value, R.string.test_basic2)) {
      Basic2();
    } else if (Match(value, R.string.test_getset)) {
      GettersSetters();
    } else if (Match(value, R.string.test_check_div_zero)) {
      CheckDivZero(0);
    } else if (Match(value, R.string.test_check_div_zero_t0)) {
      CheckDivZero(1);
    } else if (Match(value, R.string.test_check_div_zero_t3)) {
      CheckDivZero(2);
    } else if (Match(value, R.string.test_check_div_zero_t4)) {
      CheckDivZero(3);
    } else if (Match(value, R.string.test_check_div_zero_t6)) {
      CheckDivZero(4);
    } else if (Match(value, R.string.test_check_null)) {
      CheckNull();
    } else if (Match(value, R.string.test_check_outofbounds)) {
      ChecksOutOfBounds(0);
    } else if (Match(value, R.string.test_check_outofbounds_t0)) {
      ChecksOutOfBounds(1);
    } else if (Match(value, R.string.test_check_outofbounds_t1)) {
      ChecksOutOfBounds(2);
    } else if (Match(value, R.string.test_check_outofbounds_t5)) {
      ChecksOutOfBounds(3);
    } else if (Match(value, R.string.test_check_outofbounds_t6)) {
      ChecksOutOfBounds(4);
    } else if (Match(value, R.string.test_check_instanceof)) {
      CheckInstanceOf();
    } else if (Match(value, R.string.test_string)) {
      Strings();
    } else if (Match(value, R.string.test_intrinsics)) {
      Intrinsics();
    } else if (Match(value, R.string.test_arrays)) {
      Arrays();
    } else if (Match(value, R.string.test_vectors)) {
      Vectors();
    } else if (Match(value, R.string.test_invoke_quick_basic)) {
      InvokeQuickBasic();
    } else if (Match(value, R.string.test_invoke_quick_nested)) {
      InvokeQuickNested();
    } else if (Match(value, R.string.test_invoke_quick_basic_static)) {
      InvokeQuickStaticBasic();
    } else if (Match(value, R.string.test_invoke_quick_nested_static)) {
      InvokeQuickStaticNested();
    } else if (Match(value, R.string.test_exceptions)) {
      Exceptions();
    } else if (Match(value, R.string.test_trycatch)) {
      TryCatch();
    } else if (Match(value, R.string.test_newinstance)) {
      NewInstance();
    } else if (Match(value, R.string.DIVISOR_LINE)) {
      // ignore the ------
      return;
    } else {
      String msg = "Not  assigned: " + value + "!";
      Log.e(TAG, msg);
      Toast.makeText(ctx,msg, Toast.LENGTH_SHORT).show();
    }
  }
}
