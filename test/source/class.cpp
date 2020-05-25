#include <doctest/doctest.h>
#include <glue/class.h>

using namespace glue;

namespace {

  struct A {
    std::string member;
    int method(int x) { return 42 + x; }
    void ambiguousMethod(){};
    void ambiguousMethod() const {};
  };

  struct B : public A {
    int intMember = 0;
    B(std::string m) : A{m} {}
    std::string anotherMethod(const A &x, std::string sep) const { return member + sep + x.member; }
  };

}  // namespace

TEST_CASE("ClassValue") {
  auto gA = glue::createClass<A>()
                .addConstructor<>()
                .addMethod("method", &A::method)
                .addConstMethod("constMethod", &A::ambiguousMethod)
                .addNonConstMethod("nonConstMethod", &A::ambiguousMethod)
                .addMethod("lambda", [](A &a) { return a.method(3); })
                .addMember("member", &A::member);

  auto gB = glue::createClass<B>(glue::WithBases<A>())
                .setExtends(gA)
                .addConstructor<std::string>()
                .addMethod("anotherMethod", &B::anotherMethod)
                .addMember("intMember", &B::intMember);

  REQUIRE(glue::getClassInfo(*gA.data));
  CHECK(glue::getClassInfo(*gA.data)->typeID == revisited::getTypeID<A>());

  REQUIRE(glue::getClassInfo(*gB.data));
  CHECK(glue::getClassInfo(*gB.data)->typeID == revisited::getTypeID<B>());
  CHECK(gB.data["method"]);

  SUBCASE("empty instance") {
    Instance a;
    CHECK(!a);
  }

  SUBCASE("create instance of A") {
    auto va = gA.construct();
    REQUIRE(va);
    CHECK(va["method"](10).get<int>() == 52);
    CHECK(va["lambda"]().get<int>() == 45);
    CHECK_NOTHROW(va["setMember"]("Hello!"));
    CHECK(va["member"]().get<std::string>() == "Hello!");
  }

  SUBCASE("create instance of B") {
    auto vb = gB.construct("B");
    CHECK(vb->type() == revisited::getTypeID<B>());
    CHECK(vb["method"](10).get<int>() == 52);
    CHECK_NOTHROW(vb["setIntMember"](10.0));
    CHECK(vb["intMember"]().get<int>() == 10);
    CHECK(vb["anotherMethod"](*gB.construct("A"), "x").get<std::string>() == "BxA");
  }
}
