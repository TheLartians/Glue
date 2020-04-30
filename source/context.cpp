#include <easy_iterator.h>
#include <glue/class.h>
#include <glue/context.h>

using namespace glue;

void Context::addMap(const MapValue &map, std::vector<std::string> &path) {
  if (auto info = getClassInfo(map)) {
    TypeInfo typeInfo;
    typeInfo.classInfo = info;
    typeInfo.path = path;
    typeInfo.data = map;
    uniqueTypes.push_back(info->typeID);
    types[info->typeID] = typeInfo;
    types[info->constTypeID] = typeInfo;
    types[info->sharedTypeID] = typeInfo;
    types[info->sharedConstTypeID] = typeInfo;
  } else {
    map.forEach([&](auto &&key, auto &&value) {
      if (auto m = value.asMap()) {
        path.push_back(key);
        addMap(m, path);
        path.pop_back();
      }
      return false;
    });
  }
}

void Context::addRootMap(const MapValue &map) {
  Path path;
  addMap(map, path);
}

const Context::TypeInfo *Context::getTypeInfo(const TypeID &type) const {
  if (auto it = easy_iterator::find(types, type)) {
    return &it->second;
  } else {
    return nullptr;
  }
}

Instance Context::createInstance(Value value) const {
  if (auto type = getTypeInfo(value->type())) {
    if (type->classInfo->converter) {
      value = type->classInfo->converter(*value);
    }
    return Instance(type->data, value);
  } else {
    return Instance();
  }
}
