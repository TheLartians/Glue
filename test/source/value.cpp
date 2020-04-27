#include <doctest/doctest.h>
#include <glue/value.h>
#include <glue/anymap.h>

using namespace glue;

TEST_CASE("Value") {
  Value value;
  CHECK(!value.asFunction());
  CHECK(!value.asMap());
  CHECK(!value);

  value = 42;
  CHECK(!value.asFunction());
  CHECK(!value.asMap());
  CHECK(value->as<int>() == 42);

  value = [](int x){ return 42 + x; };
  REQUIRE(value.asFunction());
  CHECK(value.asFunction()(3).as<int>() == 45);
  CHECK(!value.asMap());

  value = createAnyMap();
  CHECK(!value.asFunction());
  REQUIRE(value.asMap());
  CHECK_NOTHROW(value.asMap()["x"] = 42);
  CHECK(value.asMap()["x"]->as<int>() == 42);
}

