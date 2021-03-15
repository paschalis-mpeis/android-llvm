/*
 * Copyright 2021 Paschalis Mpeis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package mp.paschalis.llvm.demo;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class TestInternalMethods {

  private static final HashMap<Character, String> movesMap;

  static {
    movesMap = new HashMap();
    movesMap.put(Character.valueOf('a'), "a1");
    movesMap.put(Character.valueOf('b'), "b1");
    movesMap.put(Character.valueOf('c'), "c1");
    movesMap.put(Character.valueOf('d'), "d1");
    movesMap.put(Character.valueOf('e'), "e1");
    movesMap.put(Character.valueOf('f'), "f1");
    movesMap.put(Character.valueOf('g'), "g1");
    movesMap.put(Character.valueOf('h'), "h1");
    movesMap.put(Character.valueOf('i'), "a2");
  }

  public static void RunTestsDebug() {
    RunTestHashmapSimple();
  }

  public static void RunTestsFull() {
    RunTestHashMapStringAndCallingMethods();
    RunTestArrayList();
    RunTestHashmapSimple();
  }
    public static void RunTestHashMapStringAndCallingMethods() {
      TestInternalMethods t = new TestInternalMethods();
      verifyHashMap(t.TestHashMap());
      t.TestCallingMethods();
    }

  public static void RunTestArrayList() {
    ArrayList<Integer> al = new ArrayList<>();
    al.add(10);
    al.add(5);
    al.add(4);
    al.add(2);
    al.add(1434);
    verifyInt("TestArrayListSize", TestArrayListSize(al), 5);
    verifyInt("TestArrayListSum", TestArrayListSum(al), 1455);
  }

  public static void RunTestHashmapSimple() {
    HashMap<Integer, Integer> m1 = new HashMap<>();
    m1.put(5,4);
    m1.put(3,2);
    m1.put(2,1);
    m1.put(1,-0);
    m1.put(7,6);
    m1.put(8,7);
    verifyInt("TestHashMapSize", TestHashMapSize1(m1), 6);

    // CHECK if it is first:
    verifyInt("TestHashMapSize:2", TestHashMapSize2(m1), 6);

    // ALSO HERE IT WORKS:
    // verifyInt("TestHashMapSize:2", TestHashMapSize2(m1), 6);
    verifyInt("TestHashMapSumKeys", TestHashMapSumKeys(m1), 26);

    verifyInt("TestHashMapSumValues", TestHashMapSumValues(m1), 20);

    // if this is LAST, then it works (before bugfix):
    // WHY?! bcz probably someone has run before... and things were populated correctly...
    // verifyInt("TestHashMapSize:2", TestHashMapSize2(m1), 6);
  }

  public static int TestArrayListSize(ArrayList<Integer> l) {
    return l.size();
  }

  public static int TestArrayListSum(ArrayList<Integer> l) {
    int s=0;
    for(int i=0; i<l.size(); i++){
      s+=l.get(i);
    }
    return s;
  }

  public static void verifyInt(String msg, int res, int correct) {
    CheckTest.Run(msg + ":", "Result: " + res, (res == correct));
  }

  public static void verifyHashMap(String res) {
    String correctResult =
      "c:a str:a1\n" +
        "c:b str:b1\n" +
        "c:c str:c1\n" +
        "c:d str:d1\n" +
        "c:e str:e1\n" +
        "c:f str:f1\n" +
        "c:g str:g1\n" +
        "c:h str:h1\n" +
        "c:i str:a2\n";
    // + "\nCorrectResult:\n" + correctResult
    CheckTest.Run("TestHashMap:", "Result:--\n" + res + "-- end of result", (res.equals(correctResult)));
  }

  public char GetKey__NOT_HOT__(Map.Entry entry) {
    return (Character) entry.getKey();
  }

  public static int TestHashMapSize1(HashMap<Integer, Integer> map) {
    return map.size();
  }


  public static int TestHashMapSize2(HashMap<Integer, Integer> map) {
    int s=0;
    Iterator localIterator = map.entrySet().iterator();
    while (localIterator.hasNext()) {
      Map.Entry entry = (Map.Entry) localIterator.next();
      s++;
    }
    return s;
  }

  public static int TestHashMapSumValues(HashMap<Integer, Integer> map) {
    int s=0;
    Iterator localIterator = map.entrySet().iterator();
    while (localIterator.hasNext()) {
      Map.Entry entry = (Map.Entry) localIterator.next();
      s+=(Integer)entry.getValue();
    }
    return s;
  }

  public static int TestHashMapSumKeys(HashMap<Integer, Integer> map) {
    int s=0;
    Iterator localIterator = map.entrySet().iterator();
    while (localIterator.hasNext()) {
      Map.Entry entry = (Map.Entry) localIterator.next();
      s+=(Integer)entry.getKey();
    }
    return s;
  }


  public String TestHashMap() {
    String res = "";
    Iterator localIterator = movesMap.entrySet().iterator();

    while (localIterator.hasNext()) {
      Map.Entry entry = (Map.Entry) localIterator.next();
      String str = (String) entry.getValue();
      // char c = GetKey__NOT_HOT__(entry) ;
      char c = (char) entry.getKey();
      res += "c:" + c + " str:" + str + "\n";
    }
    return res;
  }

  public String TestCallingMethods() {
    return foo(5) + bar(4.34f);
  }

  public String foo(int a) {
    return Integer.toString(a) + t1(a+5);
  }




   public String bar(float a) {
    return Float.toString(a);
  }





  public String t1(int a) {
    return "___" + Integer.toString(a);
  }


}
