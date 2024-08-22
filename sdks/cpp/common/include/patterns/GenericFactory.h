#pragma once

#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

/**
 * @file
 *
 * GenericFactory
 * @author John R. Naylor
 * @copyright Copyright (c) 2022, Ross Video Limited. All Rights Reserved.
 */

/** @example factory.cpp
 * Provides example of using GenericFactory.
 */

// common
#include <patterns/Singleton.h>
#include <meta/Streamable.h>


//
// C++ runtime includes
//
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <memory>

namespace catena {
namespace patterns {

/**
 * @brief Factory template with which client classes can register a "Maker"
 * function together with a unique key.
 *
 * GenericFactories are Singletons.
 *
 * @tparam P the Product made by the factory.
 * @tparam K the key that uniquely identifies the product
 * @tparam Ms parameters for the Maker function.
 * @since 1.0.0
 */
template <typename P, typename K, typename... Ms>
class GenericFactory final : public Singleton<GenericFactory<P, K, Ms...>> {
  public:
    /**
     * Type alias to function returning a pointer to a product
     * taking a variable number of arguments
     * @since 1.0.0
     */
    using Maker = P* (*)(Ms...);

    /**
     * Type alias to the Singleton's Protector type.
     * Needed for Singletons that are class templates such as
     * this one.
     *
     * C++17 needs "typename" here, C++20 doesn't.
     * @since 1.0.0
     */
    using Protector = typename Singleton<GenericFactory<P, K, Ms...>>::Protector;

  private:
    /**
     * Used to provide thread safe use of the public methods.
     */
    mutable std::mutex _mtx;
    using LockGuard = std::lock_guard<std::mutex>;

    /**
     * registry of named maker functions
     */
    std::unordered_map<K, Maker> _registry;

  public:
    /**
     * As a singleton, we needn't provide a default constructor.
     * @since 1.0.0
     */
    GenericFactory() = delete;

    /**
     * Pattern constructor called by Singleton::getInstance.
     * Using Protector as a dummy parameter prevents its use by
     * client code.
     * @since 1.0.0
     */
    explicit GenericFactory(Protector) {}

    /**
     * Registers Products that the GenericFactory can make.
     * Thread safe.
     * @param[in] key unique identifier for component.
     * @param[in] maker function to make the identified component.
     * @throws std::runtime_error if key is a duplicate.
     * @since 1.0.0
     */
    bool addProduct(const K key, Maker maker) {
        LockGuard lock(_mtx);
        auto it = _registry.find(key);
        if (it == _registry.end()) {
            _registry[key] = maker;
        } else {
            std::stringstream err;
            err << __PRETTY_FUNCTION__;
            err << ", attempted to register item with duplicate key";
            catena::meta::stream_if_possible(err, key);
            throw std::runtime_error(err.str());
        }
        return true;
    }

    /**
     * Creates a Product of type identified by key.
     * Thread safe.
     * @param[in] key unique identfier for component to be made.
     * @param[in] args parameter pack to pass to maker function
     * @return shared pointer to newly created Product
     * @throws std::runtime_error if key doesn't exist.
     * @since 1.0.0
     */
    std::unique_ptr<P> makeProduct(const K key, Ms&&... args) {
        LockGuard lock(_mtx);
        auto it = _registry.find(key);
        if (it != _registry.end()) {
            std::unique_ptr<P> ans;
            try {
                // N.B. P could be a base class with pure virtual methods, so 
                // we can't use std::make_unique here.
                P* p = it->second(std::forward<Ms>(args)...);
                ans.reset(p);
                return ans;
            }
            catch (std::exception& e) {
                // in case the product maker throws an exception
                std::stringstream err;
                err << __PRETTY_FUNCTION__;
                err << ", caught exception: ";
                err << e.what();
                throw std::runtime_error(err.str());
            }
        } else {
            std::stringstream err;
            err << __PRETTY_FUNCTION__;
            err << ", could not find entry with key: ";
            catena::meta::stream_if_possible(err, key);
            throw std::runtime_error(err.str());
        }
    }

    /**
     * Tests whether Factory can make a product with given key.
     * Thread safe.
     * @param[in] key name of product to query.
     * @return true if factory can make objects with key provided, false
     * otherwise.
     * @since 1.0.0
     */
    bool canMake(const K key) const {
        LockGuard lock(_mtx);
        bool ans = false;
        auto it = _registry.find(key);
        if (it != _registry.end()) {
            ans = true;
        }
        return ans;
    }

    /**
     * Returns list of items that the factory can make.
     * Thread safe.
     * @return vector of key values.
     * @since 1.0.0
     */
    // std::vector<K> inventory() const {
    //   LockGuard lock(_mtx);
    //   std::vector<K> ans;
    //   for (auto& it : _registry) {
    //     ans.push_back(it.first);
    //   }
    //   return ans;
    // }

    /**
     * Serializes keys registered with the factory.
     * Thread safe.
     * @return newline delimited string listing keys registered with factory.
     * @since 1.0.0
     */
    // std::string serialize() const {
    //   LockGuard lock(_mtx);
    //   std::string ans;
    //   for (auto& it : _registry) {
    //     ans = ans + it.first + "\n";
    //   }
    //   return ans;
    // }
};

}  // namespace patterns
}  // namespace catena


/**
 * Serializes an GenericFactory to std::ostream.
 * @relates rv::patterns::GenericFactory
 * @tparam P product made by GenericFactory
 * @tparam Ms optional parameter pack used by factory's maker method.
 * @todo figure out why doxygen doesn't write out the parameters.
 * @param[in-out] os stream written to.
 * @param[in] f GenericFactory to print out.
 * @return updated ostream
 * @since 1.0.0
 */
// template <typename P, typename K, typename... Ms>
// std::ostream& operator<<(std::ostream& os,
//                          const rv::patterns::GenericFactory<P, K, Ms...>& f) {
//   return os << f.serialize();
// }
