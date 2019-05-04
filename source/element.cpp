#include <glue/element.h>

using namespace glue;

AnyReference ElementMap::getValue(const std::string &key) const {
  auto it = data.find(key);
  if (it != data.end()) {
    return it->second.getValue();
  }
  it = data.find(extendsKey);
  if (it != data.end()) if (auto map = it->second.asMap()) {
    return map->getValue(key);
  }
  return AnyReference();
}

void ElementMap::setValue(const std::string &key, Any && value){
  if (!value) {
    auto it = data.find(key);
    if (it != data.end()) {
      data.erase(it);
    }
  } else {
    auto & element = data[key];
    element.setValue(std::forward<Any>(value));
  }
  onValueChanged.emit(key, value);
}

std::vector<std::string> ElementMap::keys()const{
  std::vector<std::string> result;
  for (auto it = data.begin(), end = data.end(); it != end; ++it) {
    result.push_back(it->first);
  }
  return result;
}

ElementMapEntry Map::operator[](const std::string &key){
  return Entry(shared_from_this(), key);
}

std::shared_ptr<Map> ElementInterface::asMap() const {
  return tryGet<Map>();
}

ElementMapEntry ElementInterface::operator[](const std::string &key){
  if (auto map = asMap()) {
    return (*map)[key];
  } else {
    auto &m = setToMap();
    return m[key];
  }
}

void ElementMapEntry::setValue(Any&&value) {
  return parent->setValue(key, std::forward<Any>(value));
}

AnyReference ElementMapEntry::getValue() const {
  return parent->getValue(key);
}

Map& ElementMapEntry::setToMap() {
  return set<ElementMap>();
}

void Element::setValue(Any &&value) {
  data = value;
}

AnyReference Element::getValue() const {
  return data;
}

Map& Element::setToMap() {
  return set<ElementMap>();
}
