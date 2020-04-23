#include <doctest/doctest.h>

// clang-format off
#include <glue/elements/class_element.h>

struct A {
  std::string name;
  A(const std::string &n): name(n) {}
};

struct B {
  
};

void example() {
  
  auto glueA = glue::ClassElement<A>()
  .addConstructor<std::string>()
  .addMember("name", &A::name);
  
  auto glueB = glue::ClassElement<B>()
  .setBase(glueA);
  
}
// clang-format on


TEST_CASE("Example") {
  CHECK_NOTHROW(example());
}
