#include <doctest/doctest.h>
#include <glue/array.h>

#include <vector>

using namespace glue;

TEST_CASE("Array") {
  auto arrayValue = glue::createArrayClass<std::vector<int>>();
  auto instance = arrayValue.construct();
  CHECK(instance["size"]().as<int>() == 0);
  CHECK_THROWS(instance["pop"]());
  CHECK_NOTHROW(instance["push"](1));
  CHECK_NOTHROW(instance["push"](2));
  CHECK_NOTHROW(instance["push"](3));
  CHECK(instance["size"]().as<int>() == 3);
  CHECK(instance["get"](0).as<int>() == 1);
  CHECK(instance["get"](1).as<int>() == 2);
  CHECK(instance["get"](2).as<int>() == 3);
  CHECK_THROWS(instance["get"](3));
  CHECK_THROWS(instance["set"](3, 4));
  CHECK_NOTHROW(instance["pop"]());
  CHECK(instance["size"]().as<int>() == 2);
  CHECK_NOTHROW(instance["set"](1, 4));
  CHECK(instance["get"](1).as<int>() == 4);
  CHECK_NOTHROW(instance["erase"](0));
  CHECK(instance["get"](0).as<int>() == 4);
  CHECK(instance["size"]().as<int>() == 1);
  CHECK_NOTHROW(instance["insert"](0, 0));
  CHECK(instance["size"]().as<int>() == 2);
  CHECK(instance["get"](0).as<int>() == 0);
  CHECK_NOTHROW(instance["clear"]());
  CHECK(instance["size"]().as<int>() == 0);
}
