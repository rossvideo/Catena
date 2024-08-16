#pragma once

/**
 * @file Collection.h
 * @brief Collection class for storing and retrieving items by name 
 * @author isaac.robert@rossvideo.com
 * @date 2024-08-14
 * @copyright Copyright (c) 2024 Ross Video
 */

#include <unordered_map>

namespace catena::common {
  class IConstraint;    // forward reference
} 

namespace catena {
namespace lite {

class IParam;         // forward reference
class IMenuGroup;     // forward reference
class ILanguagePack;  // forward reference

/**
 * @brief ParamTag type for addItem and getItem, and tag-dispatched methods
 */
struct ParamTag {
    using type = IParam;
};

/**
 * @brief CommandTag type for addItem and getItem, and tag-dispatched methods
 */
struct CommandTag {
    using type = IParam;
};

/**
 * @brief ConstraintTag type for addItem and getItem, and tag-dispatched methods
 */
struct ConstraintTag {
    using type = catena::common::IConstraint;
};

/**
 * @brief MenuGroupTag type for addItem and getItem, and tag-dispatched methods
 */
struct MenuGroupTag {
    using type = IMenuGroup;
};

/**
 * @brief LanguagePackTag type for addItem and getItem, and tag-dispatched methods
 */
struct LanguagePackTag {
    using type = ILanguagePack;
};

/**
 * @brief Collection provides a way to store and retrieve items by name
 * @tparam T constraint, parameter, command, menu group, or language pack
 */
template <typename T>
class Collection {
public:
    /**
     * @brief default constructor
     */
    Collection() = default;

    /**
     * @brief default destructor
     */
    virtual ~Collection() = default;

    /**
     * @brief add an item to the colllection by name
     * @param name the name of the item
     * @param item the item to add
     */
    void addItem(const std::string name, T* item) {
        collection_[name] = item;
    }

    /**
     * @brief get an item from the colllection by name
     * @param name the name of the item
     * @return the item
     */
    T* getItem(const std::string& name) const {
        auto it = collection_.find(name);
        if (it != collection_.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief get the underlying map for iteration purposes
     * @return unordered_map used by this collection
     */
    const std::unordered_map<std::string, T*>& getMap() const {
        return collection_;
    }

    /**
     * @brief check if the collection is empty
     * @return true if empty, false otherwise
     */
    bool empty() const {
        return collection_.empty();
    }

    /// temporary function to get the first item in the collection
    /// needed for param to get its constraint until paramDescriptor separation
    T* first() const {
        return collection_.begin()->second;
    }

private:
    std::unordered_map<std::string, T*> collection_;
};

}  // namespace lite
}  // namespace catena

