#pragma once

#include <glue/element.h>
#include <glue/elements/class_element.h>
#include <revisited/type_index.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace glue {
  
  using Path = std::vector<std::string>;
  
  struct ContextClass {
    ClassInfo info;
    std::shared_ptr<Element> element;
    Path path;
  };

  class Context {
  public:
    std::unordered_map<revisited::TypeIndex, ContextClass> classes;

//    void addRoot(const std::shared_ptr<Element> &element);
//    void addElement(const std::shared_ptr<Element> &element, Path &path);
  };

}  // namespace glue
