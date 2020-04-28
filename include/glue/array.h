#pragma once

#include <glue/class.h>

namespace glue {

  template <class Array, class V = typename Array::value_type> auto createArrayClass() {
    return glue::createClass<Array>()
        .addConstructor()
        .addMethod("push", [](Array &arr, V v) { arr.emplace_back(std::move(v)); })
        .addMethod("size", [](const Array &arr) { return arr.size(); })
        .addMethod("pop",
                   [](Array &arr) {
                     if (arr.size() == 0) throw std::runtime_error("cannot pop empty array");
                     arr.pop_back();
                   })
        .addMethod("get",
                   [](const Array &arr, size_t idx) {
                     if (idx >= arr.size()) {
                       throw std::runtime_error("invalid array index");
                     }
                     return arr[idx];
                   })
        .addMethod("set",
                   [](Array &arr, size_t idx, V v) {
                     if (idx >= arr.size()) {
                       throw std::runtime_error("invalid array index");
                     }
                     arr[idx] = std::move(v);
                   })
        .addMethod("erase",
                   [](Array &arr, size_t idx) {
                     if (arr.size() <= idx) throw std::runtime_error("invalid array index");
                     arr.erase(arr.begin() + idx);
                   })
        .addMethod("insert",
                   [](Array &arr, size_t idx, V v) {
                     if (arr.size() < idx) throw std::runtime_error("invalid array insert index");
                     arr.insert(arr.begin() + idx, std::move(v));
                   })
        .addMethod("clear", [](Array &arr) { arr.clear(); });
    ;
  }

}  // namespace glue