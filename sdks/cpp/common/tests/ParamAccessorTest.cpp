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
  numParam->setValueAt(50, 0);
  numParam->setValueAt(-8, 3);
  ASSERT_EQ(numArray.size(), 4);

  int32_t val = 0;
  for(int i = 0; i < numArray.size(); i++){
    numParam->getValueAt(val, i);
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
  numParam->setValueAt((float)50.5, 0);
  numParam->setValueAt((float)-8.8, 3);
  ASSERT_EQ(numArray.size(), 4);
  
  float val = 0;
  for(int i = 0; i < numArray.size(); i++){
    numParam->getValueAt(val, i);
    EXPECT_FLOAT_EQ(val, numArray[i]);
  }
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}