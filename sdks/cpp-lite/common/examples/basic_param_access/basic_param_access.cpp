
#include <param.pb.h>

#include <DeviceAccessor.h>

#include <iostream>

int main() {
    DeviceAccessor deviceAccessor;
    std::unique_ptr<catena::Value> value = deviceAccessor.getSerializedValue("/a_number");
    std::cout << "value: " << value->int32_value() << std::endl;
    return 0;
}