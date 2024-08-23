#pragma once

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

/**
 * @brief Singleton wrapper class definition
 * @file Singleton.h
 * @author John R. Naylor
 * @copyright Copyright (c) 2020, Ross Video Limited. All Rights Reserved.
 */

namespace catena {
namespace patterns {
/**
 * Class template to make Singletons.
 * @tparam T the class to acquire Singleton behaviour.
 * @since 2.0.0
 */
template <typename T> class Singleton {
  public:
    /**
   * Instance Accessor.
   * Use this to get a reference to the Singleton object.
   * Thread-safety is guaranteed by the C++ 11 standard.
   * @since 2.0.0
   */
    static T& getInstance() {
        Protector p;
        static T instance(p);
        return instance;
    }

    /**
   * Singletons cannot be copied.
   */
    Singleton(const Singleton&) = delete;

    /**
   * Singletons cannot be moved.
   */

    Singleton(Singleton&&) = delete;

    /**
   * Singletons cannot be assigned.
   */
    const Singleton& operator=(const Singleton&) = delete;

    /**
   * Singletons cannot be assigned.
   */
    Singleton& operator=(Singleton&&) = delete;

  protected:
    /**
   * Constructor is protected so that only derived classes can use it.
   */
    Singleton() = default;

    /**
   * This is a helper that allows derived classes to declare public
   * constructors that take a Protector as an argument. Because the Protector
   * is itself protected, it means that client code cannot call the derived
   * class' public constructor because Protector is inaccessible.<br/> This
   * approach avoids the necessity of derived classes declaring Singleton as a
   * friend so that it can access their (private) constructors.
   * @since 2.0.0
   */
    struct Protector {};

    /**
   *	Destructor can be overridden by derived classes.
   */
    virtual ~Singleton() = default;
};
  
}  // namespace patterns
}  // namespace catena
