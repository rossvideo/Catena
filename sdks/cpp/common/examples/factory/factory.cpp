#include <iostream>
#include <memory>
#include <utility>

#include <patterns/GenericFactory.h>

/*
 * Factories are useful to create objects based on values that are only known
 * at runtime, such as parameter values.
 *
 * In this example we create a base class, Dog, that provides an interface
 * to interact with two different types of Dog.
 *
 * The rv::GenericFactory is used to register the various types of Dog for it to
 * make. Each type of Dog is uniquely identified by a key that is used to create
 * it.
 *
 */
class Dog {
 public:
  /*
   * Instantiate a GenericFactory to make Dogs.
   * Note that we also specify the types needed by classes that derive
   * from Dog.
   *
   * Example aside, the motivation for this creation mechanism is to allow
   * video and audio buffers with a variety of memory structures be created
   * according to runtime parameter values such as width, height, Pixel
   * ordering, memory layout etc.
   */
  using Factory = catena::patterns::GenericFactory<Dog, std::string, const std::string&, const uint32_t>;

 public:
  /*
   * Base class has a default constructor.
   */
  Dog() = default;

  /*
   * Allow derived classes to override the destructor.
   */
  virtual ~Dog() { std::cout << " whimper..." << std::endl; }

  /*
   * Use defaults for copying and assigning Dogs
   */
  Dog(const Dog&) = default;

  Dog& operator=(const Dog& rhs) = default;

  Dog(Dog&&) = default;

  Dog& operator=(Dog&& rhs) = default;

  /*
   * This is the very simple interface provided by Dog
   * for derived classes to implement.
   */
  virtual void operator()() = 0;
};

/*
 * Now we have a base class, we can declare multiple types of Dog
 * that vary by template parameters.
 */
template <typename T>
class ConcreteDog : public Dog {
 private:
  /*
   * Example attributes to construct initialize
   */
  const std::string _breed;
  const uint32_t _barkiness;

  /*
   * This is the key attribute that Classes to be created via a
   * GenericFactory must declare & define.
   */
  static bool _added;

 public:
  /*
   * Implement the base class interface.
   * Which, in this case is only 1 function and an optional destructor.
   */
  void operator()() override {
    std::stringstream ans;
    ans << _breed << ": ";
    if constexpr (std::numeric_limits<T>::is_signed) {
      ans << "can go forwards and backwards. ";
    } else {
      ans << "can only go forwards. ";
    }
    for (uint32_t i = 0; i < _barkiness; ++i) {
      ans << "bark ";
    }
    std::cout << ans.str() << std::endl;
  }

  /*
   * Construct an animal derived from Dog
   */
  ConcreteDog(std::string breed, const uint32_t barkiness)
      : _breed(std::move(breed)), _barkiness(barkiness) {}

  /*
   * Print out the breed name on destruction
   */
  ~ConcreteDog() { std::cout << _breed; }

  /*
   * Factory components must define a maker function that's static,
   * and has a signature that matches Dog::Maker.
   */
  static Dog* makeOne(const std::string& breed, const uint32_t barkiness) {
    return new ConcreteDog(breed, barkiness);
  }

  /*
   * Factory components must register themselves with the factory,
   * so we made it part of the class.
   * This method generates a key based on a template parameter
   * and will register 2 different types of Dog with the factory.
   */
  static bool registerWithFactory() {
    Factory& fac = Factory::getInstance();
    std::string key;
    if constexpr (std::numeric_limits<T>::is_signed) {
      key = "dog-bidi";
    } else {
      key = "dog-unidi";
    }
    return fac.addProduct(key, makeOne);
  }
};

/*
 * instantiate some Concrete Dogs
 */
using BiDiDog = ConcreteDog<int>;
using UniDiDog = ConcreteDog<unsigned int>;

/*
 * register them with our factory
 */
template <>
bool BiDiDog::_added = BiDiDog::registerWithFactory();

template <>
bool UniDiDog::_added = UniDiDog::registerWithFactory();

/*
 * Just for fun, partially specialize the destructor for UniDiDogs
 */
template <>
UniDiDog::~ConcreteDog() {
  std::cout << _breed << " arrrroooooo!";
}

using std::cout, std::endl;
int main() {
  try {
    // get instance of the Dog Factory
    Dog::Factory& df = Dog::Factory::getInstance();

    // print its inventory
    // cout << "Dog Factory\n" << df << endl;

    cout << "\nDog Tests" << endl;

    /*
     * Create 3 dogs of different types
     */
    std::unique_ptr<Dog> labrador, jackRussell, rabid;
    labrador = df.makeProduct("dog-bidi", "Labrador", 1);
    jackRussell = df.makeProduct("dog-bidi", "Jack Russell", 4);
    rabid = df.makeProduct("dog-unidi", "Rabid dog", 8);

    /*
     * Exercise the interface for Dog
     */
    (*labrador)();
    (*jackRussell)();
    rabid->operator()();  // alternative syntax

    cout << "\nFactory Test" << endl;
    // verify that the factory cannot make goldfish.
    try {
      df.makeProduct("goldfish", "koi carp", 0);
    } catch (std::exception& why) {
      cout << "Problem: " << why.what() << endl;
    }

    cout << "\nDestructors here..." << endl;
  } catch (std::exception& why) {
    // something went wrong with the example, so exit with non-zero error code
    // this should stop the build
    cout << "\n\n*** Example failed to run!" << why.what() << "\n\n" << endl;
    return 1;
  }
  return 0;
}

/* Example Output
Dog Factory dog - unidi dog -
    bidi

        Dog Tests Labrador : can go forwards and backwards
                                 .bark Jack Russell
    : can go
          forwards and backwards.bark bark bark bark Rabid dog
    : can only go forwards.bark bark bark bark bark bark bark bark

          Factory Test Problem
    : P*
      rv::GenericFactory<P, Ms>::makeProduct(const string&, Ms...)
          [with P = Dog;
           Ms = {const std::__cxx11::basic_string<char, std::char_traits<char>,
                                                  std::allocator<char> >&,
                 const unsigned int};
           std::__cxx11::string = std::__cxx11::basic_string<char>],
    could not find entry with key : goldfish

                                    Destructors here... Rabid dog arrrroooooo
                                    !whimper... Jack Russell whimper
                                    ... Labrador whimper...
*/
