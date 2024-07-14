#include <iostream>


#include <patterns/EnumDecorator.h>

#define COUNTER (Counter, int32_t, (one) "one", (two) "two")
INSTANTIATE_ENUM(ENUMDECORATOR_DECLARATION, COUNTER);
INSTANTIATE_ENUM(ENUMDECORATOR_DEFINITION, COUNTER);

int main () {
  Counter c1(Counter_e::one);
  std::cout << "c1: " << c1.toString() << std::endl;

  Counter c2("two");
  std::cout << "c2: " << int32_t(c2) << std::endl;
}
