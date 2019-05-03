#include <glue/element.h>

using namespace glue;

Element & Element::operator=(AnyFunction && f){
  data.set<AnyFunction>(f);
  return *this;
}

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
  if (auto element = getElement(key)) {
    return *element;
  } else {
    return data[key];
  }
}

ElementMap::Entry ElementMap::operator[](const std::string &key){
  return Entry(this, key);
}

Element &ElementMap::Entry::getOrCreateElement(){
  return parent->getOrCreateElement(key);
}

ElementMapEntry Element::operator[](const std::string &key){
  if (auto map = data.tryGet<ElementMap>()) {
    return (*map)[key];
  } else {
    data.set<ElementMap>();
    return (*this)[key];
  }
}

ElementMapEntry ElementMapEntry::operator[](const std::string &key){
  auto &entry = getOrCreateElement();
  if(auto map = entry.asMap()){
    return (*map)[key];
  } else {
    entry.data.set<ElementMap>();
    parent->onValueChanged.emit(key, entry);
    return (*this)[key];
  }
}
