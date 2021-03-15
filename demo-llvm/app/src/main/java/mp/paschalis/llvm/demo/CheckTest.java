package mp.paschalis.llvm.demo;

import android.util.Log;

import java.util.ArrayList;

import static mp.paschalis.llvm.demo.Debug.TAG;
/**
 * Created by paschalis on 05/07/2017.
 */

public class CheckTest {

  static int testsTotal=0;
  static int testsPassed=0;
  static ArrayList<String> failedLogs = new ArrayList<>();


  // check that might die
  public static void RunCheck() {
    testsTotal++;
  }
  // check that might die
  public static void RunCheckPassed(String msg) {
    testsPassed++;
    Log.i(TAG, msg);
  }

  public static void RunAndroidTestFailed(String msg) {
    testsTotal++;
    Log.e(TAG, "FAILED: " + msg);
  }


  public static void RunAndroidTestPassedSilent(String msg) {
    testsPassed++;
    testsTotal++;
  }

  public static void RunAndroidTestPassed(String msg) {
    RunAndroidTestPassedSilent(msg);
    Log.i(TAG, "PASSED: " + msg);
  }

  public static void Run(String name, String info, boolean result) {
    testsTotal++;
    if(result){
      testsPassed++;
      Log.i(TAG, name + " " + info + ": PASSED.");
    } else {
      String msg =name + " " + info + ": FAILED!" ;
      Log.e(TAG, msg);
      failedLogs.add(msg);
    }
  }


  public static void Run(String name, String info, String correctInfo, boolean result) {
    testsTotal++;
    if(result){
      testsPassed++;
      Log.i(TAG, name + " " + info + ": PASSED.");
    } else {
      String msg =name + " " + info + " (correct: '" + correctInfo +"')" + ": FAILED!" ;
      Log.e(TAG, msg);
      failedLogs.add(msg);
    }
  }

  public static void Reset() {
    testsTotal =0;
    testsPassed=0;
  }

  public static String GetReport() {
    return "TEST REPORT: " + testsPassed + "/" +testsTotal;
  }

  public static String GetErrors() {
    String errMsg = "";
    if(HasFailedTests()) {
      for(String error: failedLogs) {
        errMsg=error+"\n";
      }
    }
    return errMsg;
  }

  public static boolean HasFailedTests() {
    return (testsTotal!=testsPassed);
  }

  public static void ShowReport() {
    String msg = GetReport();
    if(!HasFailedTests()) {
      Log.i(TAG, msg);
    } else {
      Log.i(TAG, msg);
      for(String error: failedLogs) {
        Log.e(TAG, error);
      }
    }
  }
}
