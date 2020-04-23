#pragma once

#include <glue/element.h>
#include <glue/element_map_entry.h>
#include <glue/keys.h>

namespace glue {

  /**
   * A self-contained element
   */
  class AnyElement : public Element {
  private:
    Any data;

  public:
    AnyElement() = default;
    AnyElement(const AnyElement &) = default;
    AnyElement(AnyElement &&) = default;
    AnyElement(const Element &e) : data(e.getValue()) {}
    AnyElement &operator=(const AnyElement &) = default;
    AnyElement &operator=(AnyElement &&) = default;

    // non-function constructor
    template <class T, typename = typename std::enable_if<
                           !detail::is_callable<T>::value
                           && !std::is_base_of<Element, typename std::decay<T>::type>::value>::type>
    AnyElement(T &&v) : data(std::forward<T>(v)) {}

    // function constructor
    AnyElement(AnyFunction &&f) : data(f) {}

    using Element::operator=;

    Any getValue() const final override;
    void setValue(Any &&) final override;
    Map &setToMap() final override;

    /**
     * Add values by call chaining
     * Note: overrided classes should implement this method
     */
    template <class T> AnyElement &addValue(const std::string &key, T &&value);
  };

  inline void setExtends(Element &extends, const Element &element) {
    extends[keys::extendsKey] = element;
  }

}  // namespace glue
