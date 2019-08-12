#include <glue/array_element.h>
#include <vector>

#include <catch2/catch.hpp>

TEST_CASE("array element") {
  auto Array = glue::ArrayElement<std::vector<int>>();
  glue::ClassElementContext context;
  context.addElement(Array);

  auto array = context.bind(Array[glue::keys::constructorKey]());
  CHECK(array["size"]()->get<int>() == 0);
  CHECK_NOTHROW(array["push"](42));
  REQUIRE(array["size"]()->get<int>() == 1);
  CHECK(array["get"](0)->get<int>() == 42);
  CHECK_NOTHROW(array["push"](3.141)); // implicit conversion to int
  REQUIRE(array["size"]()->get<int>() == 2);
  CHECK(array["get"](1)->get<int>() == 3);
  CHECK_NOTHROW(array["set"](0,-3));
  CHECK(array["get"](0)->get<int>() == -3);
  CHECK_NOTHROW(array["insert"](1,5));
  REQUIRE(array["size"]()->get<int>() == 3);
  CHECK(array["get"](1)->get<int>() == 5);
  CHECK_NOTHROW(array["erase"](0));
  REQUIRE(array["size"]()->get<int>() == 2);
  CHECK(array["get"](0)->get<int>() == 5);
  CHECK(array["get"](1)->get<int>() == 3);

  SECTION("pop"){
    CHECK_NOTHROW(array["pop"]());
    REQUIRE(array["size"]()->get<int>() == 1);
  }

  SECTION("clear"){
    CHECK_NOTHROW(array["clear"]());
    REQUIRE(array["size"]()->get<int>() == 0);
  }

}
