#pragma once

#include <concepts>
#include <type_traits>

namespace jc {

template <template <typename...> class, template <typename...> class>
struct is_same_template : std::false_type {};

template <template <typename...> class T>
struct is_same_template<T, T> : std::true_type {};

template <template <typename...> class T, template <typename...> class U>
concept is_same_template_v = is_same_template<T, U>::value;

}  // namespace jc
