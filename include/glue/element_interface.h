#pragma once

#include <glue/detail/interface.h>
#include <glue/map.h>
#include <glue/types.h>
#include <observe/event.h>
#include <revisited/any.h>
#include <revisited/any_function.h>

#include <initializer_list>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace glue {

  /**
   * This class creates the main interface when working with elements.
   */
  class Element {
  public:
    /**
     * change the stored value to a mapped type
     */
    virtual Map &setToMap() = 0;

    /**
     * get the stored value
     */
    virtual Any getValue() const = 0;

    /**
     * set the stored value
     */
    virtual void setValue(Any &&) = 0;

    template <class T, typename = typename std::enable_if<
                           !std::is_base_of<Element, typename std::decay<T>::type>::value>::type>
    Element &operator=(T &&value) {
      if constexpr (detail::is_callable<T>::value) {
        setValue(Any::create<AnyFunction>(value));
      } else {
        setValue(std::forward<T>(value));
      }
      return *this;
    }

    Element() = default;
    Element(const Element &) = default;
    Element(Element &&) = default;
    virtual ~Element() {}

    Element &operator=(const Element &e) {
      setValue(e.getValue());
      return *this;
    }

    /**
     * Call the element if it holds an AnyFunction
     * Will throw an exception otherwise
     */
    template <typename... Args> Any operator()(Args &&... args) const {
      return getValue().get<AnyFunction>()(
          detail::convertArgument<Args>(std::forward<Args>(args))...);
    }

    /**
     * Call the element if it holds an AnyFunction
     * Will throw an exception otherwise
     */
    template <class T, typename... Args> auto &set(Args &&... args) {
      Any value;
      auto &res = value.set<T>(std::forward<Args>(args)...);
      setValue(std::move(value));
      return res;
    }

    /**
     * Returnes the element's value casted to `T`, if possible
     * Will throw an exception otherwise
     */
    template <class T> T get() const { return getValue().get<T>(); }

    /**
     * Returnes the element's value casted to `T *`, if possible
     * Will return `nullptr` otherwise
     */
    template <class T> std::shared_ptr<T> tryGet() const { return getValue().getShared<T>(); }

    /**
     * returns a pointer to the map stored by the element if it holds one, and an empty pointer
     * otherwise
     */
    std::shared_ptr<Map> asMap() const;

    /**
     * sets the value to a `Map` type and returns the entry at position `key`
     */
    ElementMapEntry operator[](const std::string &key);

    /**
     * returns `true` if the element holds a value and `false` otherwise
     */
    explicit operator bool() const { return bool(getValue()); }
  };

}  // namespace glue
