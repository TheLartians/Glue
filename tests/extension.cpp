#include <catch2/catch.hpp>
#include <stdexcept>

#include <lars/glue.h>

TEST_CASE("Extension"){
  using namespace lars;

  auto extension = std::make_shared<Extension>();
  extension->add_function("get", [](){ return 42; });
  REQUIRE(extension->get_function("get")().get_numeric<int>() == 42);
  extension->add_function("getAny", [](){ return make_any<int>(42); });
  REQUIRE(extension->get_function("getAny")().get_numeric<int>() == 42);

  extension->add_function("add", [](int a, int b){ return a + b; });
  REQUIRE_THROWS(extension->get_function("add")());
  REQUIRE_THROWS(extension->get_function("add")(2));
  REQUIRE(extension->get_function("add")(2,3).get_numeric<int>() == 5);
  REQUIRE_THROWS(extension->get_function("add")(2,3,4));

  extension->add_function("addAny", [](Any a, Any b){ return a.get_numeric() + b.get_numeric(); });
  REQUIRE(extension->get_function("addAny")(2,3.5).get_numeric() == Approx(5.5));

}