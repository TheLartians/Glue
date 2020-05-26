#pragma once

#include <glue/class.h>
#include <glue/keys.h>
#include <glue/value.h>

namespace glue {

  template <class T> struct EnumGenerator : public ValueBase {
    MapValue data = createAnyMap();

    EnumGenerator() {
      setClassInfo<T>(data);
      data[keys::operators::eq] = [](T a, T b) { return a == b; };
    }

    EnumGenerator &addValue(const std::string &key, T value) {
      data[key] = value;
      return *this;
    }

    operator MapValue() const { return data; }
  };

  template <class T> auto createEnum() { return EnumGenerator<T>(); }

}  // namespace glue
