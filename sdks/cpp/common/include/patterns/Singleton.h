#pragma once

/*
 * Copyright 2024 Ross Video Ltd
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
 * @brief Singleton wrapper class definition
 * @file Singleton.h
 * @author John R. Naylor
 * @copyright Copyright (c) 2024, Ross Video Limited. All Rights Reserved.
 */

namespace catena {
namespace patterns {

/**
 * @brief Class template to make Singletons.
 * @tparam T the class to acquire Singleton behaviour.
 * @since 2.0.0
 */
template <typename T> class Singleton {
  public:
    /**
     * @brief Instance Accessor.
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
     * @brief Singletons cannot be copied.
     */
    Singleton(const Singleton&) = delete;

    /**
     * @brief Singletons cannot be moved.
     */
    Singleton(Singleton&&) = delete;

    /**
     * @brief Singletons cannot be assigned.
     */
    const Singleton& operator=(const Singleton&) = delete;

    /**
     * @brief Singletons cannot be assigned.
     */
    Singleton& operator=(Singleton&&) = delete;

  protected:
    /**
     * @brief Constructor is protected so that only derived classes can use it.
     */
    Singleton() = default;

    /**
     * @brief This is a helper that allows derived classes to declare public
     * constructors that take a Protector as an argument. Because the Protector
     * is itself protected, it means that client code cannot call the derived
     * class' public constructor because Protector is inaccessible.<br/> This
     * approach avoids the necessity of derived classes declaring Singleton as a
     * friend so that it can access their (private) constructors.
     * @since 2.0.0
     */
    struct Protector {};

    /**
     * @brief Destructor can be overridden by derived classes.
     */
    virtual ~Singleton() = default;
};
  
}  // namespace patterns
}  // namespace catena
