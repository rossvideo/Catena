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
