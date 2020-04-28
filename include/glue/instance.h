#pragma once

#include <glue/value.h>

#include <cassert>

namespace glue {

  struct Instance : public Value {
    MapValue classMap;

    Instance(){};
    Instance(MapValue c, Value v) : Value(std::move(v)), classMap(std::move(c)) {}

    auto operator[](const std::string &key) const {
      return [=](auto &&... args) {
        assert(*this);
        return classMap[key].asFunction()(**this, std::forward<decltype(args)>(args)...);
      };
    }
  };

}  // namespace glue