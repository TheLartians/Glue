#pragma once

#include <revisited/any.h>
#include <revisited/any_function.h>

namespace glue {

  using Any = revisited::Any;
  using AnyFunction = revisited::AnyFunction;

  struct Map : public revisited::Visitable<Map> {
    virtual Any get(const std::string &) const = 0;
    virtual void set(const std::string &, const Any &) = 0;
    virtual bool forEach(std::function<bool(const std::string &, const Any &)>) const = 0;
  };

}  // namespace  glue
