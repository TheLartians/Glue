#include <glue/class_element.h>
#include <iostream>

using namespace glue;

void ClassElementContext::addMap(const std::shared_ptr<glue::Map> &map) {
  if (auto C = (*map)[classKey]) {
    types[C.get<lars::TypeIndex>()] = map;
  }

  for (auto &key: map->keys()) {
    if (auto m = (*map)[key].asMap()) {
      addMap(m);
    }
  }
}

void ClassElementContext::addElement(const ElementInterface &e) {
  if (auto m = e.asMap()) {
    addMap(m);
  }
}

std::shared_ptr<glue::Map> ClassElementContext::getMapForType(const lars::TypeIndex &idx)const {
  auto it = types.find(idx);
  if (it != types.end()) {
    return it->second;
  } else {
    return std::shared_ptr<glue::Map>();
  }
}

BoundAny ClassElementContext::bind(lars::Any && v)const {
  return BoundAny(*this, std::move(v));
}
