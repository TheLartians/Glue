#include <glue/class_element.h>
#include <glue/declarations.h>
#include <sstream>
#include <iostream>
#include <catch2/catch.hpp>

TEST_CASE("declarations") {

  struct A {
    int data;
    explicit A(int x):data(x){}
    A add(const A &other){ return A(other.data + data); }
  };

  struct B: public A {
    std::string name;
    using A::A;
    std::string description() const {
      return name + ": " + std::to_string(data);
    }
  };

  glue::ClassElement<A> AElement = glue::ClassElement<A>()
  .addConstructor<int>()
  .addMember("data", &A::data)
  .addMethod("add", &A::add)
  .addMethod("custom", [](const A &a){ return a.data+1; })
  .addMethod("variadic", [](const lars::AnyArguments &){ return 0; })
  ;

  glue::ClassElement<B> BElement = glue::ClassElement<B>()
  .addConstructor<int>()
  .addMember("name", &B::name)
  .addMethod("add", &A::add)
  .addConstMethod("description", &B::description)
  .setExtends(AElement)
  ;

  glue::Element constants;
  constants["x"] = 42;
  constants["a"] = A(1);
  constants["b"] = B(2);

  glue::Element elements;
  elements["A"] = AElement;
  elements["B"] = BElement;
  elements["constants"] = constants;

  CHECK(glue::getTypescriptDeclarations("elements", elements) == 
R"(declare module elements {
  /** @customConstructor elements.A.__new */
  class A {
    constructor(arg0: number)
    add(arg1: elements.A): elements.A;
    custom(): number;
    data(): number;
    setData(arg1: number): void;
    variadic(...args: any[]): number;
  }
  /** @customConstructor elements.B.__new */
  class B extends elements.A {
    constructor(arg0: number)
    add(arg1: elements.A): elements.A;
    description(): string;
    name(): string;
    setName(arg1: string): void;
  }
  module constants {
    let a: elements.A;
    let b: elements.B;
    let x: number;
  }
}
)");
}
