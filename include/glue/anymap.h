#pragma once

#include <glue/map.h>
#include <glue/value.h>

#include <unordered_map>

namespace glue {

  struct AnyMap : public Map {
    std::unordered_map<std::string, Any> data;
    Any get(const std::string &key) const;
    void set(const std::string &key, const Any &value);
    bool forEach(const std::function<bool(const std::string &, const Any &)> &callback) const;
  };

}  // namespace glue