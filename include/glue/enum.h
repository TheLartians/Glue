#pragma once

#include <glue/class.h>
#include <glue/keys.h>
#include <glue/value.h>
#include <type_traits>

namespace glue {

  template <class T> struct EnumGenerator : public ValueBase {
    MapValue data = createAnyMap();

    EnumGenerator() {
      setClassInfo<T>(data);
      data[keys::operators::eq] = [](const T &a, const T &b) { return a == b; };
      data["value"] = [](const T &a) { return static_cast<typename std::underlying_type<T>::type>(a); };
    }

    EnumGenerator &addValue(const std::string &key, T value) {
      data[key] = value;
      return *this;
    }

    operator MapValue() const { return data; }
  };

  template <class T> auto createEnum() { return EnumGenerator<T>(); }

}  // namespace glue
