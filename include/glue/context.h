#pragma once

#include <glue/element.h>
#include <revisited/type_index.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace glue {

  struct ContextClass {
    std::shared_ptr<Element> element;
    std::vector<std::string> types;
    revisited::TypeID typeID;
  };

  class Context {
  public:
    std::unordered_map<revisited::TypeIndex, ContextClass> classes;
  };

}  // namespace glue
