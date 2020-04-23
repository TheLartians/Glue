#pragma once

#include <glue/element.h>
#include <unordered_map>
#include <revisited/type_index.h>
#include <vector>
#include <string>

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

}
