#include <doctest/doctest.h>
#include <glue/context.h>
#include <glue/declarations.h>
#include <glue/value.h>

#include <iostream>

namespace {

  struct A {
    std::string member;
  };

  struct B : public A {
    B(std::string m) : A{m} {}
    auto method() { return 42; }
  };

}  // namespace

TEST_CASE("Declarations") {
  auto root = glue::createAnyMap();

  root["A"]
      = glue::createClass<A>()
            .addConstructor<>()
            .addMember("member", &A::member)
            .addMethod("staticMethod", [](int x) { return x * 0.5f; })
            .addMethod("sharedMethod", [](const std::shared_ptr<A> &a, const std::string &other) {
              return a->member + other;
            });

  root["B"] = glue::createClass<B>(glue::WithBases<A>())
                  .addConstructor<std::string>()
                  .addMethod("method", &B::method)
                  .setExtends(root["A"]);

  root["createB"] = []() { return B("B"); };
  root["createBWithArgument"] = [](const std::string &name) { return B(name); };

  glue::Context context;
  context.addRootMap(root);

  glue::DeclarationPrinter printer;
  printer.init();

  printer.print(std::cout, root, &context);
  std::cout << std::endl;
}
