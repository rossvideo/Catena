#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/** Copyright 2024 Ross Video Ltd

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
//

/**
 * @brief Provides tools to create maps of functions.
 * @file Functory.h
 * @author john.naylor@rossvideo.com
 * @copyright © 2024 Ross Video Ltd.
 */

// common
#include <patterns/Singleton.h>
#include <meta/Streamable.h>

#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <functional>


namespace catena {
namespace patterns {
            
/**
 * @brief Functory template to store function objects that
 * can then be retrieved by a key.
 * 
 * Functory is a Singleton.
 * 
 * @tparam K the key type.
 * @tparam R the return type.
 * @tparam Ms the function argument types.
 * 
 * 
*/
template <typename K, typename R, typename... Ms>
class Functory final : public Singleton<Functory<K, R, Ms...>> {
public:
    /**
     * Type alias to function object taking a variable number of
     * arguments, returning R
    */
    using Function = std::function<R(Ms...)>;

    /**
     * Type alias to the Singleton's Protector type.
     * Needed for Singletons that are class templates such as
     * this one.
     * 
    */
   using Protector = typename catena::patterns::Singleton<Functory<K, R, Ms...>>::Protector;

  private:
    /**
     * Used to provide thread safe use of the public methods.
     */
    mutable std::mutex _mtx;
    using LockGuard = std::lock_guard<std::mutex>;

    /**
     * registry of functions, indexed by Key
     */
    std::unordered_map<K, Function> _registry;

  public:
    /**
     * As a singleton, we don't provide a default constructor.
     * @since 1.0.0
     */
    Functory() = delete;

    /**
     * Pattern constructor called by Singleton::getInstance.
     * Using Protector as a dummy parameter prevents its use by
     * client code.
     * @since 1.0.0
     */
    Functory(Protector) {}

    /**
     * Registers Products that the GenericFactory can make.
     * Thread safe.
     * @param[in] key unique identifier for component.
     * @param[in] maker function to make the identified component.
     * @throws std::runtime_error if key is a duplicate.
     * @since 1.0.0
     */
    bool addFunction(const K key, Function f) {
        LockGuard lock(_mtx);
        auto it = _registry.find(key);
        if (it == _registry.end()) {
            _registry[key] = f;
        } else {
            std::stringstream err;
            err << __PRETTY_FUNCTION__;
            err << ", attempted to register item with duplicate key";
            catena::meta::stream_if_possible (err, key);
            throw std::runtime_error(err.str());
        }
        return true;
    }

    /**
     * Returns the function registered with the given key.
     * Thread safe.
     * @param[in] key unique identfier for component to be made.
     * @return shared pointer to newly created Product
     * @throws std::runtime_error if key doesn't exist.
     * @since 1.0.0
     */
    const Function& operator[](const K key) const {
        LockGuard lock(_mtx);
        auto it = _registry.find(key);
        if (it != _registry.end()) {
            return it->second;
        } else {
            std::stringstream err;
            err << __PRETTY_FUNCTION__ << ", key not found ";
            catena::meta::stream_if_possible (err, key);
            throw std::runtime_error(err.str());
        }
    }

    /**
     * returns true if the key is in the registry
     * @param key - the key to look for
    */
    bool has(const K key) const {
        LockGuard lock(_mtx);
        return _registry.contains(key);
    }
};
}
}