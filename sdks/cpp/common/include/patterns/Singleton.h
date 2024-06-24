#pragma once

/**
 * @file
 *
 * Singleton
 * @author John R. Naylor
 * @copyright Copyright (c) 2020, Ross Video Limited. All Rights Reserved.
 *
 * This is Confidential material per the terms of any Non Disclosure Agreement
 * currently in force between you and Ross Video Limited. You must treat it as
 * such.
 */

/** @example  ../examples/Singleton/SingletonExample.cpp
 * This is a comprehensively worked example that shows how to make Singletons.
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
