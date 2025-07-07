/*
 * Copyright 2025 Ross Video Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * RE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @brief Example program to demonstrate the GenericFactory class
 * @file factory.cpp
 * @copyright Copyright © 2025 Ross Video Ltd
 * @author John R. Naylor (john.naylor@rossvideo.com)
 */

// common
#include <patterns/GenericFactory.h>
#include <Logger.h>
#include <iostream>
#include <memory>
#include <utility>

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
  virtual ~Dog() { DEBUG_LOG << " whimper..."; }

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
    DEBUG_LOG << ans.str();
  }

  /*
   * Construct an animal derived from Dog
   */
  ConcreteDog(std::string breed, const uint32_t barkiness)
      : _breed(std::move(breed)), _barkiness(barkiness) {}

  /*
   * Print out the breed name on destruction
   */
  ~ConcreteDog() { DEBUG_LOG << _breed; }

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
  DEBUG_LOG << _breed << " arrrroooooo!";
}

int main() {
  FLAGS_logtostderr = false;          // Keep logging to files
  FLAGS_log_dir = GLOG_LOGGING_DIR;   // Set the log directory
  google::InitGoogleLogging("factory");
  
  try {
    // get instance of the Dog Factory
    Dog::Factory& df = Dog::Factory::getInstance();

    // print its inventory
    // DEBUG_LOG << "Dog Factory\n" << df << endl;

    DEBUG_LOG << "Dog Tests";

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

    DEBUG_LOG << "Factory Test";
    // verify that the factory cannot make goldfish.
    try {
      df.makeProduct("goldfish", "koi carp", 0);
    } catch (std::exception& why) {
      DEBUG_LOG << "Problem: " << why.what();
    }

    DEBUG_LOG << "Destructors here...";
  } catch (std::exception& why) {
    // something went wrong with the example, so exit with non-zero error code
    // this should stop the build
    DEBUG_LOG << "*** Example failed to run!" << why.what();
    return 1;
  }
  
    // Shutdown Google Logging
    google::ShutdownGoogleLogging();
    return 0;
}

/* Possible Output
Dog Tests
Labrador: can go forwards and backwards. bark 
Jack Russell: can go forwards and backwards. bark bark bark bark 
Rabid dog: can only go forwards. bark bark bark bark bark bark bark bark 

Factory Test
Problem: std::unique_ptr<P> catena::patterns::GenericFactory<Dog, std::string, const std::string &, const unsigned int>::makeProduct(const K, Ms &&...) [P = Dog, K = std::string, Ms = <const std::string &, const unsigned int>], could not find entry with key: goldfish

Destructors here...
Rabid dog arrrroooooo! whimper...
Jack Russell whimper...
Labrador whimper...
*/
