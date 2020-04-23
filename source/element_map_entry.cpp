#include <glue/element_map_entry.h>
#include <glue/element_map.h>

using namespace glue;

ElementMapEntry &ElementMapEntry::operator=(const ElementMapEntry &other) {
  Element::operator=(other);
  return *this;
}

std::shared_ptr<Map> Element::asMap() const { return tryGet<Map>(); }

ElementMapEntry Element::operator[](const std::string &key) {
  if (auto map = asMap()) {
    return (*map)[key];
  } else {
    auto &m = setToMap();
    return m[key];
  }
}

void ElementMapEntry::setValue(Any &&value) {
  return parent->setValue(key, std::forward<Any>(value));
}

Any ElementMapEntry::getValue() const { return parent->getValue(key); }

Map &ElementMapEntry::setToMap() { return set<ElementMap>(); }
