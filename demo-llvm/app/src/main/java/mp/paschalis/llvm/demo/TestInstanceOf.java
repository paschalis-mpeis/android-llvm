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

import mp.paschalis.llvm.demo.TestInheritance.Audi;
import mp.paschalis.llvm.demo.TestInheritance.Car;
import mp.paschalis.llvm.demo.TestInheritance.Engine;
import mp.paschalis.llvm.demo.TestInheritance.Nissan;
import mp.paschalis.llvm.demo.TestInheritance.Vehicle;

public class TestInstanceOf {

  static int i;
  final static int all=14; // num of test methods here
  private static String GetInfo() {
    return  ++i + "/" + all + ": ";
  }
  public static void RunTests() {
    i=0;
    TestInstanceOf t = new TestInstanceOf();
    Audi a = new Audi();
    Car c = new Car();
    Vehicle n = new Nissan();

    verifyInstanceOf(GetInfo() + "Audi Is Audi", t.InstanceOfAudi(a), true);
    verifyInstanceOf(GetInfo() + "Audi Is Car", t.InstanceOfCar(a), true);
    verifyInstanceOf(GetInfo() + "Audi Is Vehicle", t.InstanceOfVehicle(a), true);

    verifyInstanceOf(GetInfo() + "Car Is Audi", t.InstanceOfAudi(c), false);
    verifyInstanceOf(GetInfo() + "Car Is Car", t.InstanceOfCar(c), true);
    verifyInstanceOf(GetInfo() + "Car Is Vehicle", t.InstanceOfVehicle(c), true);


    verifyInstanceOf(GetInfo() + "Nissan Is Audi", t.InstanceOfAudi(n), false);
    verifyInstanceOf(GetInfo() + "Nissan Is Car", t.InstanceOfCar(n), true);
    verifyInstanceOf(GetInfo() + "Nissan Is Vehicle", t.InstanceOfVehicle(n), true);
    verifyInstanceOf(GetInfo() + "Nissan Is Nissan", t.InstanceOfNissan(n), true);
    verifyInstanceOf(GetInfo() + "Audi Is Nissan", t.InstanceOfNissan(a), false);


    // interfaceof
    verifyInstanceOf(GetInfo() + "Audi Is Engine", t.InterfaceOfEngine(a), true);
    verifyInstanceOf(GetInfo() + "Car Is Engine", t.InterfaceOfEngine(c), true);
    verifyInstanceOf(GetInfo() + "Nissan Is Engine", t.InterfaceOfEngine(n), true);
  }

  public static void verifyInstanceOf(String info, boolean res, boolean correct_res) {
    CheckTest.Run("TestInstanceOf:", info + ": " + res, res == correct_res);
  }

  public boolean InstanceOfNissan(Vehicle a) {
    return a instanceof Nissan;
  }

  public boolean InstanceOfAudi(Vehicle a) {
    return a instanceof Audi;
  }

  public boolean InstanceOfCar(Vehicle a) {
    return a instanceof Car;
  }

  public boolean InstanceOfVehicle(Engine a) {
    return a instanceof Vehicle;
  }


  public boolean InterfaceOfEngine(Object a) {
    return a instanceof Engine;
  }
}
