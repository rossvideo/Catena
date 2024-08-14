#include <functional>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <iostream>


class Param; // forward declaration
class Constraint; // forward declaration

// function objects passed to ctors of Param and Constraint
using ParamAdd = std::function<void(const std::string& key, Param* item)>;
using ConstraintAdd = std::function<void(const std::string& key, Constraint* item)>;

class Param{
public:
    /**
     * @brief Construct a new Param object
     * @param oid the oid of the param
     * @param p the function object to add the param to its parent
     */
    Param(const std::string& oid, ParamAdd add) {
        add(oid, this);
    }
};


class Constraint{
public:
    Constraint(const std::string& oid, ConstraintAdd add) {
        add(oid, this);
    }
};

struct ParamTag {using type = Param;};
struct ConstraintTag {using type = Constraint;};



class Device {
public:
    
    /**
     * generic function to add an item to the device
     */
    template<typename TAG>
    void addItem(const std::string& key, typename TAG::type* item){
        if constexpr (std::is_same_v<TAG, ParamTag>) {
            params_[key] = item;
        }
        if constexpr (std::is_same_v<TAG, ConstraintTag>) {
            constraints_[key] = item;
        }
    }

    /**
     * generic function to list items of a specific type
     */
    template <typename TAG>
    void listItems(TAG t) {
        if constexpr (std::is_same_v<TAG, ParamTag>) {
            for (const auto& pair: params_) {
                std::cout << "key: " << pair.first << ", address: " << static_cast<void*>(pair.second) << std::endl;
            }
        }
        if constexpr (std::is_same_v<TAG, ConstraintTag>) {
            for (const auto& pair: constraints_) {
                std::cout << "key: " << pair.first << ", address: " << static_cast<void*>(pair.second) << std::endl;
            }
        }
        
    }

    /**
     * specific function to add a param to the device
     */
    void addParam(const std::string& key, Param* p) {
        params_[key] = p;
    }
private:
    std::unordered_map<std::string, Param*> params_;
    std::unordered_map<std::string, Constraint*> constraints_;
};


int main () {
    Device dev{};
    using std::placeholders::_1;
    using std::placeholders::_2;
    ParamAdd paramAdder = std::bind(&Device::addParam, &dev, _1, _2);
    ParamAdd itemAdder = std::bind(&Device::addItem<ParamTag>, &dev, _1, _2);
    
    /**
     * construction of Param objects is better because we don't
     * expose the internal state of the device.
     */
    Param wowParam("wow", itemAdder);
    Param helloParam("hello", paramAdder);
    Param zowParam("zow", itemAdder);

    dev.listItems(ParamTag{});

    ConstraintAdd constraintAdder = std::bind(&Device::addItem<ConstraintTag>, &dev, _1, _2);
    Constraint wowConstraint("cow", constraintAdder);
    dev.listItems(ConstraintTag{});


}