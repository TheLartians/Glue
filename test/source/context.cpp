#include <doctest/doctest.h>
#include <glue/class.h>
#include <glue/context.h>
#include <glue/value.h>

namespace {

  struct A {
    std::string member;
  };

  struct B : public A {
    B(std::string m) : A{m} {}
    auto method() { return 42; }
  };

}  // namespace

TEST_CASE("Context") {
  auto root = glue::createAnyMap();

  root["A"] = glue::createClass<A>().addConstructor<>().addMember("member", &A::member);

  root["B"] = glue::createClass<B>(glue::WithBases<A>())
                  .addConstructor<std::string>()
                  .addMethod("method", &B::method)
                  .setExtends(root["A"]);

  root["createB"] = []() { return B("B"); };

  glue::Context context;
  context.addRootMap(root);

  REQUIRE(context.getTypeInfo(glue::getTypeID<A>()));
  REQUIRE(context.getTypeInfo(glue::getTypeID<B>()));
  CHECK(context.getTypeInfo(glue::getTypeID<A>())->path == glue::Context::Path{"A"});
  CHECK(context.getTypeInfo(glue::getTypeID<B>())->path == glue::Context::Path{"B"});

  auto instance = context.createInstance(root["createB"].asFunction()());

  REQUIRE(instance);
  CHECK(instance["method"]().get<int>() == 42);
  CHECK(instance["member"]().get<std::string>() == "B");
}
