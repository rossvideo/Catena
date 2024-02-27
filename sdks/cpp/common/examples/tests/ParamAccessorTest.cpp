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

class Int32AccessorTest: public ::testing::Test {
  protected:
  DeviceModel dm = DeviceModel("../../../example_device_models/device.one_of_everything.json");
  
};

TEST_F(Int32AccessorTest, Int32Getter){
  std::unique_ptr<ParamAccessor> numParam = dm.param("/a_number");
  int32_t num = 0;
  numParam->getValue(num);
  EXPECT_EQ(num, 1234);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}