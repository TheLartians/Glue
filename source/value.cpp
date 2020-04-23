#include <glue/value.h>

using namespace glue2;

MapValue Value::asMap() const {
  return MapValue{data.getShared<Map>()};
}

std::optional<AnyFunction> Value::asFunction() const {
  auto f = data.tryGet<AnyFunction>();
  if (f) {
    return *f;
  } else {
    return std::nullopt;
  }
}
