#include <doctest/doctest.h>
#include <glue/instance.h>
#include <glue/keys.h>
#include <glue/value.h>

using namespace glue;

TEST_CASE("MapValue") {
  auto map = createAnyMap();

  CHECK(!map["value"]);
  REQUIRE_NOTHROW(map["value"] = 3);
  REQUIRE(map["value"]);
  CHECK(map["value"]->as<int>() == 3);

  REQUIRE_NOTHROW(map["function"] = [](int x) { return 42 + x; });
  CHECK(map["function"].asFunction());
  CHECK(map["function"].asFunction()(3).as<int>() == 45);

  CHECK(map.keys() == std::vector<std::string>{"function", "value"});
}

TEST_CASE("Extended map") {
  auto base = createAnyMap();
  base["a"] = 1;
  base["b"] = 2;

  auto map = createAnyMap();

  SUBCASE("no extends") {
    CHECK(!map["a"]);
    CHECK(!map["b"]);
  }

  SUBCASE("extends map") {
    map.setExtends(base);
    map["a"] = 3;
    CHECK(map["a"]->as<int>() == 3);
    CHECK(map["b"]->as<int>() == 2);
  }

  SUBCASE("extends callback") {
    map.setExtends([](const glue::MapValue &, std::string key) { return key; });
    CHECK(map["a"]->as<std::string>() == "a");
    CHECK(map["b"]->as<std::string>() == "b");
  }

  SUBCASE("recursive extends") {
    map.setExtends(base);
    map["a"] = 3;
    auto map2 = createAnyMap();
    map2["a"] = 4;
    map2.setExtends(map);
    CHECK(map2["a"]->as<int>() == 4);
    CHECK(map2["b"]->as<int>() == 2);
  }
}