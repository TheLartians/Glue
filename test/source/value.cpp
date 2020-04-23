#include <doctest/doctest.h>
#include <glue/value.h>

#include <unordered_map>
#include <easy_iterator.h>

using namespace glue2;

struct AnyMap: public Map {
  std::unordered_map<std::string, Any> data;

  Any get(const std::string &key) const {
    if (auto it = easy_iterator::find(data, key)) {
      return it->second;
    } else {
      return Any();
    }
  }

  void set(const std::string &key, const Any &value) {
    data[key] = value;
  }

  bool forEach(std::function<bool(const std::string &, const Any &)> callback) const {
    for (auto && v: data) {
      if (callback(v.first, v.second)) return true;
    }
    return false;
  }

};

TEST_CASE("value") {
  Value value;
  *value = 42;

  CHECK(!value.asFunction());
  CHECK(!value.asMap());
  CHECK(value->get<int>() == 42);

  MapValue map{std::make_shared<AnyMap>()};
  CHECK(!map["value"]);
  REQUIRE_NOTHROW(map["value"] = 3);
  REQUIRE(map["value"]);
  CHECK(map["value"]->get<int>() == 3);
}
