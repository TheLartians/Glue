#include <doctest/doctest.h>
#include <glue/anymap.h>
#include <glue/value.h>

using namespace glue;

TEST_CASE("Value") {
  Value value;

  SUBCASE("empty") {
    CHECK(!value.asFunction());
    CHECK(!value.asMap());
    CHECK(!value);
  }

  SUBCASE("scalar") {
    value = 42;
    CHECK(!value.asFunction());
    CHECK(!value.asMap());
    CHECK(value->as<int>() == 42);
  }

  SUBCASE("function") {
    value = [](int x) { return 42 + x; };
    REQUIRE(value.asFunction());
    CHECK(value.asFunction()(3).as<int>() == 45);
    CHECK(!value.asMap());
  }

  SUBCASE("map") {
    value = createAnyMap();
    CHECK(!value.asFunction());
    REQUIRE(value.asMap());
    CHECK_NOTHROW(value.asMap()["x"] = 42);
    CHECK(value.asMap()["x"]->as<int>() == 42);
  }
}

TEST_CASE("Simplified value interface") {
  Value inner = glue::createAnyMap();
  inner["a"] = [](int x) { return 42 + x; };
  Value root = glue::createAnyMap();
  root["inner"] = inner;
  root["value"] = 42;

  Value view;
  CHECK_THROWS(view["inner"]);

  view = Value(*root);
  CHECK(view["value"]->get<int>() == 42);
  CHECK(!*view["xxx"]);
  CHECK_THROWS(view["inner"]());
  CHECK_NOTHROW(view["inner"]);
  CHECK_NOTHROW(view["inner"]["a"]);

  CHECK(view["inner"]["a"](6)->get<int>() == 48);
  CHECK(view["inner"]["a"].asFunction().call(glue::AnyArguments{8}).get<int>() == 50);
}
