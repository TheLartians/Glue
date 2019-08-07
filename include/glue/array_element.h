#pragma once

#include <glue/element.h>
#include <exception>

namespace glue {

  template <class T, class V = typename T::value_type> Element ArrayElement() {
    return glue::ClassElement<T>()
    .addConstructor()
    .addMethod("push", [](T &arr, V v){ arr.emplace_back(v); })
    .addMethod("size", [](T &arr){ return arr.size(); })
    .addMethod("pop", [](T &arr){ 
      if (arr.size() == 0) throw std::runtime_error("cannot pop empty array"); 
      arr.pop_back(); 
    })
    .addMethod("get", [](const T &arr, size_t idx){ 
      if (idx > arr.size()) {
        throw std::runtime_error("invalid array index");
      }
      return arr[idx];
    })
    .addMethod("set", [](Node::ChildArray &arr, size_t idx, V v){
      if (idx > arr.size()) {
        throw std::runtime_error("invalid array index");
      } 
      arr[idx] = v;
    })
    ;
  };

}