#include <glue/element.h>
#include <easy_iterator.h>

using namespace glue;
using easy_iterator::eraseIfFound;
using easy_iterator::found;

AnyReference ElementMap::getValue(const std::string &key) const {
  if (auto v = found(data.find(key), data)) {
    return v->second.getValue();
  } else if (auto v = found(data.find(keys::extendsKey), data)) {
    if (auto map = v->second.asMap()) {
      return map->getValue(key);
    }
  }
  return AnyReference();
}

void ElementMap::setValue(const std::string &key, Any && value){
  if (!value) {
    eraseIfFound(data.find(key), data);
    eraseIfFound(elementObservers.find(key), elementObservers);
  } else {
    if (key == keys::classKey) {
      auto & type = value.get<const lars::TypeIndex &>();
      addClass(type, shared_from_this());
    }
    if (auto *map = value.tryGet<ElementMap>()) {
      for (auto c: map->classes) {
        addClass(c.first, c.second);
      }
      elementObservers.emplace(key, map->onClassAdded.createObserver([this](auto &type, auto &map){
        addClass(type, map);
      }));
    }
    auto & element = data[key];
    element.setValue(std::forward<Any>(value));
  }
  onValueChanged.emit(key, value);
}

void ElementMap::addClass(const lars::TypeIndex &type, const std::shared_ptr<Map> &map){
  auto alreadyAdded = found(classes.find(type), classes);
  classes.emplace(type, map);
  if (!alreadyAdded) {
    onClassAdded.emit(type, map);
  }
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

Element &Element::addValue(const std::string &key, Any&&value) {
  (*this)[key] = std::move(value);
  return *this;
}
