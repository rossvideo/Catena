// Licensed under the Creative Commons Attribution NoDerivatives 4.0
// International Licensing (CC-BY-ND-4.0);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
// https://creativecommons.org/licenses/by-nd/4.0/
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <gtest/gtest.h>

#include <DeviceModel.h>
#include <ParamAccessor.h>
#include <Path.h>
#include <Reflect.h>
#include <utils.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <typeinfo>
#include <typeindex>
#include <variant>

using Index = catena::Path::Index;
using DeviceModel = catena::DeviceModel;
using Param = catena::Param;
using ParamAccessor = catena::ParamAccessor;

class ParamAccessorTest: public ::testing::Test {
  protected:
  DeviceModel dm = DeviceModel("../../../example_device_models/device.one_of_everything.json");

};

TEST_F(ParamAccessorTest, Int32Access){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/a_number");
  int32_t num = 0;
  numParam->getValue(num);
  EXPECT_EQ(num, 1234);
  num = 5678;
  numParam->setValue(num);
  num = 0;
  numParam->getValue(num);
  EXPECT_EQ(num, 5678);
}

TEST_F(ParamAccessorTest, Float32Access){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/float_example");
  float num = 0;
  numParam->getValue(num);
  EXPECT_FLOAT_EQ(num, 1234.5678);
  num = 5678.1234;
  numParam->setValue(num);
  num = 0;
  numParam->getValue(num);
  EXPECT_FLOAT_EQ(num, 5678.1234);
}

TEST_F(ParamAccessorTest, StringAccess){
  std::unique_ptr<ParamAccessor> strParam = dm.param("/string_example");
  std::string str;
  strParam->getValue(str);
  EXPECT_EQ(str, "Hello, World!");
  std::string newStr = "Goodbye, World!";
  strParam->setValue(newStr);
  EXPECT_EQ(str, "Hello, World!");
  str = "";
  strParam->getValue(str);
  EXPECT_EQ(str, "Goodbye, World!");
}

TEST_F(ParamAccessorTest, Int32ArrayAccessTest){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/number_array");
  std::vector<int32_t> numArray;
  std::vector<int32_t> otherArray = {1, 2, 3, 4};
  numParam->getValue(numArray);
  ASSERT_EQ(numArray.size(), 4);
  for(int i = 0; i < numArray.size(); i++){
    EXPECT_EQ(numArray[i], otherArray[i]);
  }

  numArray = {5, 6, 7, 8};
  numParam->setValue(numArray);
  numParam->getValue(otherArray);
  ASSERT_EQ(otherArray.size(), 4);
  for(int i = 0; i < numArray.size(); i++){
    EXPECT_EQ(otherArray[i], numArray[i]);
  }

  numArray[0] = 50;
  numArray[3] = -8;
  numParam->setValue(50, 0);
  numParam->setValue(-8, 3);
  ASSERT_EQ(numArray.size(), 4);

  int32_t val = 0;
  for(int i = 0; i < numArray.size(); i++){
    numParam->getValue(val, i);
    EXPECT_EQ(val, numArray[i]);
  }
}

TEST_F(ParamAccessorTest, Float32ArrayAccessTest){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/float_array");
  std::vector<float> numArray;
  std::vector<float> otherArray = {1.1, 2.2, 3.3, 4.4};
  numParam->getValue(numArray);
  ASSERT_EQ(numArray.size(), 4);
  for(int i = 0; i < numArray.size(); i++){
    EXPECT_FLOAT_EQ(numArray[i], otherArray[i]);
  }

  numArray = {5.5, 6.6, 7.7, 8.8};
  numParam->setValue(numArray);
  numParam->getValue(otherArray);
  ASSERT_EQ(otherArray.size(), 4);
  for(int i = 0; i < numArray.size(); i++){
    EXPECT_FLOAT_EQ(otherArray[i], numArray[i]);
  }

  numArray[0] = 50.5;
  numArray[3] = -8.8;
  numParam->setValue((float)50.5, 0);
  numParam->setValue((float)-8.8, 3);
  ASSERT_EQ(numArray.size(), 4);
  
  float val = 0;
  for(int i = 0; i < numArray.size(); i++){
    numParam->getValue(val, i);
    EXPECT_FLOAT_EQ(val, numArray[i]);
  }
}

TEST_F(ParamAccessorTest, StringArrayAccessTest){
  std::unique_ptr<ParamAccessor> strParam = dm.param("/string_array");
  std::vector<std::string> strArray;
  std::vector<std::string> otherArray = {"one", "two", "three", "four"};
  strParam->getValue(strArray);
  ASSERT_EQ(strArray.size(), 4);
  for(int i = 0; i < strArray.size(); i++){
    EXPECT_EQ(strArray[i], otherArray[i]);
  }

  strArray = {"five", "six", "seven", "eight"};
  strParam->setValue(strArray);
  strParam->getValue(otherArray);
  ASSERT_EQ(otherArray.size(), 4);
  for(int i = 0; i < strArray.size(); i++){
    EXPECT_EQ(otherArray[i], strArray[i]);
  }

  strArray[0] = "nine";
  strParam->setValue(strArray[0], 0);
  strArray[3] = "ten";
  strParam->setValue(strArray[3], 3);
  ASSERT_EQ(strArray.size(), 4);
  
  std::string val;
  for(int i = 0; i < strArray.size(); i++){
    strParam->getValue(val, i);
    EXPECT_EQ(val, strArray[i]);
  }
}

TEST_F(ParamAccessorTest, Int32ValueAccess){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/a_number");
  catena::Value numValue;
  numParam->getValue(&numValue);
  EXPECT_EQ(numValue.int32_value(), 1234);
  numValue.set_int32_value(5678);
  numParam->setValue("test", numValue);
  numParam->getValue(&numValue);
  EXPECT_EQ(numValue.int32_value(), 5678);
}

TEST_F(ParamAccessorTest, Float32ValueAccess){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/float_example");
  catena::Value numValue;
  numParam->getValue(&numValue);
  EXPECT_FLOAT_EQ(numValue.float32_value(), 1234.5678);
  numValue.set_float32_value(5678.1234);
  numParam->setValue("test", numValue);
  numParam->getValue(&numValue);
  EXPECT_FLOAT_EQ(numValue.float32_value(), 5678.1234);
}

TEST_F(ParamAccessorTest, StringValueAccess){
  std::unique_ptr<ParamAccessor> strParam = dm.param("/string_example");
  catena::Value strValue;
  strParam->getValue(&strValue);
  EXPECT_EQ(strValue.string_value(), "Hello, World!");
  strValue.set_string_value("Goodbye, World!");
  strParam->setValue("test", strValue);
  strParam->getValue(&strValue);
  EXPECT_EQ(strValue.string_value(), "Goodbye, World!");
}

TEST_F(ParamAccessorTest, Int32ValueArrayAccessTest){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/number_array");
  catena::Value numValueArray;
  std::string context = "test";

  //test getting whole array
  numParam->getValue(&numValueArray);
  std::vector<int32_t> otherArray = {1, 2, 3, 4};
  ASSERT_EQ(numValueArray.int32_array_values().ints_size(), 4);
  for(int i = 0; i < numValueArray.int32_array_values().ints_size(); i++){
    EXPECT_EQ(numValueArray.int32_array_values().ints(i), otherArray[i]);
  }

  //test setting whole array
  numValueArray.mutable_int32_array_values()->clear_ints();
  for (int i = 5; i < 9; i++){
    numValueArray.mutable_int32_array_values()->add_ints(i);
  }
  otherArray = {5, 6, 7, 8};
  numParam->setValue(context, numValueArray);
  numValueArray.mutable_int32_array_values()->clear_ints();
  ASSERT_EQ(numValueArray.int32_array_values().ints_size(), 0);
  numParam->getValue(&numValueArray);
  ASSERT_EQ(numValueArray.int32_array_values().ints_size(), 4);
  for(int i = 0; i < numValueArray.int32_array_values().ints_size(); i++){
    EXPECT_EQ(numValueArray.int32_array_values().ints(i), otherArray[i]);
  }

  //test getAt and setAt
  catena::Value val;
  std::vector<std::string> scopes = {AUTHZ_DISABLED};
  val.set_int32_value(50);
  numParam->setValue(context, val, 0, scopes);
  val.set_int32_value(-8);
  numParam->setValue(context, val, 3, scopes);
  otherArray = {50, 6, 7, -8};
  for(int i = 0; i < numValueArray.int32_array_values().ints_size(); i++){
    numParam->getValue(&val, i, scopes);
    EXPECT_EQ(val.int32_value(), otherArray[i]);
  }
}

TEST_F(ParamAccessorTest, Float32ValueArrayAccessTest){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/float_array");
  catena::Value numValueArray;
  std::string context = "test";

  //test getting whole array
  numParam->getValue(&numValueArray);
  std::vector<float> otherArray = {1.1, 2.2, 3.3, 4.4};
  ASSERT_EQ(numValueArray.float32_array_values().floats_size(), 4);
  for(int i = 0; i < numValueArray.float32_array_values().floats_size(); i++){
    EXPECT_FLOAT_EQ(numValueArray.float32_array_values().floats(i), otherArray[i]);
  }

  //test setting whole array
  numValueArray.mutable_float32_array_values()->clear_floats();
  for (float i = 5.5; i < 9; i += 1.1){
    numValueArray.mutable_float32_array_values()->add_floats(i);
  }
  otherArray = {5.5, 6.6, 7.7, 8.8};
  numParam->setValue(context, numValueArray);
  numValueArray.mutable_float32_array_values()->clear_floats();
  ASSERT_EQ(numValueArray.float32_array_values().floats_size(), 0);
  numParam->getValue(&numValueArray);
  ASSERT_EQ(numValueArray.float32_array_values().floats_size(), 4);
  for(int i = 0; i < numValueArray.float32_array_values().floats_size(); i++){
    EXPECT_FLOAT_EQ(numValueArray.float32_array_values().floats(i), otherArray[i]);
  }

  //test getAt and setAt
  catena::Value val;
  std::vector<std::string> scopes = {AUTHZ_DISABLED};
  val.set_float32_value(50.5);
  numParam->setValue(context, val, 0, scopes);
  val.set_float32_value(-8.8);
  numParam->setValue(context, val, 3, scopes);
  otherArray = {50.5, 6.6, 7.7, -8.8};
  for(int i = 0; i < numValueArray.float32_array_values().floats_size(); i++){
    numParam->getValue(&val, i, scopes);
    EXPECT_FLOAT_EQ(val.float32_value(), otherArray[i]);
  }
}

TEST_F(ParamAccessorTest, StringValueArrayAccessTest){
  std::unique_ptr<ParamAccessor> strParam = dm.param("/string_array");
  catena::Value strValueArray;
  std::string context = "test";

  //test getting whole array
  strParam->getValue(&strValueArray);
  std::vector<std::string> otherArray = {"one", "two", "three", "four"};
  ASSERT_EQ(strValueArray.string_array_values().strings_size(), 4);
  for(int i = 0; i < strValueArray.string_array_values().strings_size(); i++){
    EXPECT_EQ(strValueArray.string_array_values().strings(i), otherArray[i]);
  }

  //test setting whole array
  strValueArray.mutable_string_array_values()->clear_strings();
  strValueArray.mutable_string_array_values()->add_strings("five");
  strValueArray.mutable_string_array_values()->add_strings("six");
  strValueArray.mutable_string_array_values()->add_strings("seven");
  strValueArray.mutable_string_array_values()->add_strings("eight");
  
  otherArray = {"five", "six", "seven", "eight"};
  strParam->setValue(context, strValueArray);
  strValueArray.mutable_string_array_values()->clear_strings();
  ASSERT_EQ(strValueArray.string_array_values().strings_size(), 0);
  strParam->getValue(&strValueArray);
  ASSERT_EQ(strValueArray.string_array_values().strings_size(), 4);
  for(int i = 0; i < strValueArray.string_array_values().strings_size(); i++){
    EXPECT_EQ(strValueArray.string_array_values().strings(i), otherArray[i]);
  }

  //test getAt and setAt
  catena::Value val;
  std::vector<std::string> scopes = {AUTHZ_DISABLED};
  val.set_string_value("nine");
  strParam->setValue(context, val, 0, scopes);
  val.set_string_value("ten");
  strParam->setValue(context, val, 3, scopes);
  otherArray = {"nine", "six", "seven", "ten"};
  for(int i = 0; i < strValueArray.string_array_values().strings_size(); i++){
    strParam->getValue(&val, i, scopes);
    EXPECT_EQ(val.string_value(), otherArray[i]);
  }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}