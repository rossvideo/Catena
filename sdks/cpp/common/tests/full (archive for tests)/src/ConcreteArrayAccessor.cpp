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

#include <ArrayAccessor.h>
#include <TypeTraits.h>

using catena::full::ConcreteArrayAccessor;


// the implementations for simple types are very similar, so we use a macro to DRY
#define INSTANTIATE(TYPE, values, arr_size, set_method, get_method) \
    template <>                                               \
    catena::Value ConcreteArrayAccessor<TYPE>::operator[](std::size_t idx) const { \
        auto &arr = _in.get().values();                       \
        if (arr.arr_size() >= idx) {                          \
            catena::Value ans{};                              \
            ans.set_method(arr.get_method(idx));              \
            return ans;                                       \
        } else {                                              \
            std::stringstream err;                            \
            err << "Index is out of range: " << idx << " >= " << arr.arr_size(); \
            BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE); \
        }                                                     \
    }

INSTANTIATE(float, float32_array_values, floats_size, set_float32_value, floats);
INSTANTIATE(int, int32_array_values, ints_size, set_int32_value, ints);
INSTANTIATE(std::string, string_array_values, strings_size, set_string_value, strings);

// struct implementation
template <> catena::Value ConcreteArrayAccessor<catena::StructList>::operator[](std::size_t idx) const {
    auto &arr = _in.get().struct_array_values();

    if (arr.struct_values_size() > idx) {
        auto &sv = arr.struct_values(idx);

        catena::StructValue out{};
        catena::Value ans{};

        out.mutable_fields()->insert(sv.fields().begin(), sv.fields().end());
        *(ans.mutable_struct_value()) = out;

        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.struct_values_size();
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
    }
}


// variant implementation
template <> catena::Value ConcreteArrayAccessor<catena::StructVariantList>::operator[](std::size_t idx) const {
    auto &arr = _in.get().struct_variant_array_values();

    if (arr.struct_variants_size() > idx) {
        catena::Value ans{};
        *(ans.mutable_struct_variant_value()) = arr.struct_variants(idx);
        return ans;
    } else {
        std::stringstream err;
        err << "Index is out of range: " << idx << " >= " << arr.struct_variants_size();
        BAD_STATUS(err.str(), catena::StatusCode::OUT_OF_RANGE);
    }
}
