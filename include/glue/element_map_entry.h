#pragma once

#include <glue/element.h>

namespace glue {

  /**
   * a referenced element that will set its parent's value if updated.
   * Note: if the parent is destroyed while the reference is active the behaviour is undefined.
   */
  class ElementMapEntry : public Element {
  private:
    Map *parent;
    std::string key;
    AnyElement &getOrCreateElement();

  public:
    ElementMapEntry(Map *p, const std::string &k) : parent(p), key(k) {}
    ElementMapEntry(const ElementMapEntry &) = default;
    ElementMapEntry(ElementMapEntry &&) = default;

    using Element::operator=;
    ElementMapEntry &operator=(const ElementMapEntry &other);

    Any getValue() const final override;
    void setValue(Any &&) final override;
    Map &setToMap() final override;
  };

}  // namespace glue
