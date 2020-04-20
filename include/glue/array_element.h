#pragma once

#include <glue/class_element.h>

#include <exception>

namespace glue {

  template <class T, class V = typename T::value_type> Element ArrayElement() {
    return glue::ClassElement<T>()
        .addConstructor()
        .addMethod("push", [](T &arr, V v) { arr.emplace_back(std::move(v)); })
        .addMethod("size", [](const T &arr) { return arr.size(); })
        .addMethod("pop",
                   [](T &arr) {
                     if (arr.size() == 0) throw std::runtime_error("cannot pop empty array");
                     arr.pop_back();
                   })
        .addMethod("get",
                   [](const T &arr, size_t idx) {
                     if (idx >= arr.size()) {
                       throw std::runtime_error("invalid array index");
                     }
                     return arr[idx];
                   })
        .addMethod("set",
                   [](T &arr, size_t idx, V v) {
                     if (idx >= arr.size()) {
                       throw std::runtime_error("invalid array index");
                     }
                     arr[idx] = std::move(v);
                   })
        .addMethod("erase",
                   [](T &arr, size_t idx) {
                     if (arr.size() <= idx) throw std::runtime_error("invalid array index");
                     arr.erase(arr.begin() + idx);
                   })
        .addMethod("insert",
                   [](T &arr, size_t idx, V v) {
                     if (arr.size() < idx) throw std::runtime_error("invalid array insert index");
                     arr.insert(arr.begin() + idx, std::move(v));
                   })
        .addMethod("clear", [](T &arr) { arr.clear(); });
  }

}  // namespace glue