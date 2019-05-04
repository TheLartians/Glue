#include <glue/element.h>

using namespace glue;

Element *ElementMap::getElement(const std::string &key){
  auto it = data.find(key);
  if (it != data.end()) {
    return &it->second;
  }
  it = data.find(extendsKey);
  if (it != data.end()) if (auto map = it->second.asMap()) {
    return map->getElement(key);
  }
  return nullptr;
}

Element &ElementMap::getOrCreateElement(const std::string &key){
  return data[key];
}

ElementMap::Entry ElementMap::operator[](const std::string &key){
  return Entry(this, key);
}

ElementMap * ElementInterface::asMap() const {
  return getValue().tryGet<ElementMap>();
}

ElementMapEntry ElementInterface::operator[](const std::string &key){
  if (auto map = asMap()) {
    return (*map)[key];
  } else {
    set<ElementMap>();
    return (*this)[key];
  }
}

