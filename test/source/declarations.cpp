#include <doctest/doctest.h>
#include <glue/context.h>
#include <glue/declarations.h>
#include <glue/value.h>

#include <sstream>

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
                  .setExtends(inner["A"]);

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
  CAPTURE(stream.str());
  CHECK(stream.str() == R"(
/** @customConstructor B.__new */
declare class B extends inner.A {
  constructor(arg0: string)
  method(): number
}
declare const createA: (this: void) => inner.A
declare module inner {
  /** @customConstructor inner.A.__new */
  class A {
    constructor()
    member(): string
    setMember(arg1: string): void
    sharedMethod(arg1: string): string
    static staticMethod(this: void): number
  }
  const createB: (this: void) => B
  const createBWithArgument: (this: void, arg0: string) => B
  const value: string
}
declare const value: number
)");
}
