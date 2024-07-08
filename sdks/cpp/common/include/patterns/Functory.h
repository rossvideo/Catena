#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @file
 * A factory for function objects.
 * 
*/

#include <common/include/patterns/Singleton.h>
#include <common/include/meta/Streamable.h>

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
        return _registry.find(key) != _registry.end();
    }
};
}
}