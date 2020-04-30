#pragma once

#include <glue/class.h>
#include <glue/instance.h>
#include <glue/map.h>
#include <glue/value.h>

#include <unordered_map>
#include <vector>

namespace glue {

  class Context {
  public:
    using Path = std::vector<std::string>;

    struct TypeInfo {
      MapValue data;
      Path path;
      std::shared_ptr<const ClassInfo> classInfo;
    };

    std::unordered_map<TypeIndex, TypeInfo> types;
    std::vector<TypeID> uniqueTypes;

    const TypeInfo *getTypeInfo(TypeIndex type) const;
    void addRootMap(const MapValue &map);
    void addMap(const MapValue &map, std::vector<std::string> &path);

    Instance createInstance(Value value) const;
  };

}  // namespace glue