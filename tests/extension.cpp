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

TEST_CASE("Automatic Casting"){
  struct A:lars::Visitable<A>{ int value = 0; };
  struct B:lars::DVisitable<B, A>{ B(){ value = 1; } };
  struct C:lars::DVisitable<C, A>{ C(){ value = 2; } };

  lars::Extension extension;
  extension.add_function("add", [](double x,double y){ return x+y; });
  REQUIRE( extension.get_function("add")(2,3).get_numeric<int>() == 5 );
  extension.add_function("add_A", [](A & x,A & y){ A a; a.value = x.value+y.value; return a; });
  REQUIRE( extension.get_function("add_A")(B(),C()).get<A>().value == 3 );
}
