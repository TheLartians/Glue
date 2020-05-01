#include <doctest/doctest.h>
#include <glue/context.h>
#include <glue/declarations.h>
#include <glue/enum.h>
#include <glue/value.h>

#include <sstream>
#include <regex>

namespace {

  struct A {
    std::string member;
  };

  struct B : public A {
    B(std::string m) : A{m} {}
    auto method() { return 42; }
  };

  enum class E { A, B, C };

}  // namespace

TEST_CASE("Declarations") {
  auto root = glue::createAnyMap();
  root["value"] = 42;

  auto inner = glue::createAnyMap();
  inner["value"] = "42";

  inner["A"]
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
                  .addValue("value", E::A)
                  .setExtends(inner["A"]);

  inner["E"] = glue::createEnum<E>().addValue("A", E::A).addValue("B", E::B).addValue("C", E::C);

  inner["createB"] = []() { return B("B"); };
  inner["createBWithArgument"] = [](const std::string &name) { return B(name); };

  root["createA"] = []() { return A(); };
  root["inner"] = inner;

  glue::Context context;
  context.addRootMap(root);

  glue::DeclarationPrinter printer;
  printer.init();

  std::stringstream stream;
  stream << '\n';
  printer.print(stream, root, &context);
  stream << '\n';

  auto declarations = stream.str();
  CAPTURE(declarations);

  // TODO: add a more thorough check of the declarations 
  CHECK(declarations.find("declare class B extends inner.A") != std::string::npos);
  CHECK(declarations.find("method(): number") != std::string::npos);
  CHECK(declarations.find("constructor(arg0: string)") != std::string::npos);
  CHECK(declarations.find("static value: inner.E") != std::string::npos);
  CHECK(declarations.find("/** @customConstructor inner.A.__new */") != std::string::npos);
  CHECK(declarations.find("static A: inner.E") != std::string::npos);
  CHECK(declarations.find("declare let value: number") != std::string::npos);
  CHECK(declarations.find("static staticMethod(this: void): number") != std::string::npos);
  CHECK(declarations.find("const createBWithArgument: (this: void, arg0: string) => B") != std::string::npos);
}
