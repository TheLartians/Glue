#include <glue/context.h>
#include <glue/elements/class_element.h>
#include <glue/keys.h>

using namespace glue;

// void Context::addRoot(const std::shared_ptr<Element> &element) {
//   Path path;
//   addElement(element, path);
// }

//void Context::addElement(const std::shared_ptr<Element> &element, Path &path) {
//  if (auto &&info = getClassInfo(*element)) {
//    classes[info->typeID] = ContextClass{info, element, path};
//  }
//  
//  if (auto && map = element.asMap()) {
//    map->forEach([](auto &&key, auto &&){
//      if (key == keys::classKey) {
//        
//      }
//      return false;
//    });
//  }
//}
//
