#include <doctest/doctest.h>
#include <glue/class.h>

using namespace glue;

namespace {
  
  struct A {
    std::string member;
    int method(int x) { return 42 + x; }
  };
  
  struct B: public A {
    B(std::string m): A{m}{}
    std::string anotherMethod(const A &x, std::string sep) const { return member + sep + x.member; }
  };

}

TEST_CASE("ClassValue") {
  auto gA = glue::createClass<A>()
  .addConstructor<>()
  .addMethod("method", &A::method)
  .addMethod("lambda", [](A &a){ return a.method(3); })
  .addMember("member", &A::member)
  ;
  
  REQUIRE(glue::getClassInfo(*gA.data));
  CHECK(glue::getClassInfo(*gA.data)->typeID == revisited::getTypeID<A>());
}
