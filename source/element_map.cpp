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

std::vector<std::string> ElementMap::keys() const {
  std::vector<std::string> result;
  for (auto it = data.begin(), end = data.end(); it != end; ++it) {
    result.push_back(it->first);
  }
  return result;
}

ElementMapEntry Map::operator[](const std::string &key) { return ElementMapEntry(this, key); }
