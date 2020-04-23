#pragma once

#include <type_traits>
#include <utility>
#include <glue/types.h>

namespace glue {

    namespace detail {
    /**
     * detect callable types.
     * source: https://stackoverflow.com/questions/15393938/
     */
    template <class...> using void_t = void;
    template <class T> using has_opr_t = decltype(&T::operator());
    template <class T, class = void> struct is_callable : std::false_type {};
    template <class T> struct is_callable<T, void_t<has_opr_t<typename std::decay<T>::type>>>
        : std::true_type {};

    template <class T>
    typename std::enable_if<std::is_base_of<Element, typename std::decay<T>::type>::value,
                            revisited::Any>::type
    convertArgument(T &&arg) {
      return revisited::Any(arg.getValue());
    }
    
    template <class T>
    typename std::enable_if<!std::is_base_of<Element, typename std::decay<T>::type>::value,
                            T &&>::type
    convertArgument(T &&arg) {
      return std::forward<T>(arg);
    }

  }  // namespace detail

}
