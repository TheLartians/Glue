#include <easy_iterator.h>
#include <glue/element_map.h>

using namespace glue;
using easy_iterator::eraseIfFound;
using easy_iterator::found;

Any ElementMap::getValue(const std::string &key) const {
  if (auto v = found(data.find(key), data)) {
    return v->second.getValue();
  } else if (extends) {
    return extends->getValue(key);
  } else {
    return Any();
  }
}

void ElementMap::setValue(const std::string &key, Any &&value) {
  if (key == keys::extendsKey) {
    extends = AnyElement(value).asMap();
  }
  if (!value) {
    eraseIfFound(data.find(key), data);
    eraseIfFound(elementObservers.find(key), elementObservers);
  } else {
    auto &element = data[key];
    element.setValue(std::forward<Any>(value));
  }
  onValueChanged.emit(key, value);
}

bool ElementMap::forEach(std::function<bool(const std::string &, const Any &)> callback) const {
  for (auto &&v: data) {
    if (callback(v.first, v.second)) return true;
  }
  return false;
}

ElementMapEntry Map::operator[](const std::string &key) { return ElementMapEntry(this, key); }
