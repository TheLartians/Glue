#include <glue/class_element.h>
#include <catch2/catch.hpp>

namespace {

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
    std::string description() {
      return "non const " + name + ": " + std::to_string(data);
    }
  };

}

template <> struct lars::AnyVisitable<B> {
  using type = lars::DataVisitableWithBases<B,A>;
};

TEST_CASE("class element") {

  glue::ClassElement<A> AElement = glue::ClassElement<A>()
  .addConstructor<int>("create")
  .addMember("data", &A::data)
  .addMethod("add", &A::add)
  .addFunction("custom", [](const A &a){ return a.data+1; })
  ;

  glue::ClassElement<B> BElement = glue::ClassElement<B>()
  .addConstructor<int>("create")
  .addMember("name", &B::name)
  .addConstMethod("description", &B::description)
  .setExtends(AElement)
  ;

  SECTION("single element"){
    glue::ClassElementContext context;
    context.addElement(AElement);

    auto a = context.bind(AElement["create"](42));

    CHECK(AElement["data"](*a).get<int>() == 42);
    CHECK(a["data"]()->get<int>() == 42);
    CHECK_NOTHROW(a["setData"](3));
    CHECK(a["data"]()->get<int>() == 3);
    CHECK(a["custom"]()->get<int>() == 4);

    auto b = context.bind(AElement["create"](5));
    CHECK(a["add"](*b)["data"]()->get<int>() == 8);
  }

  SECTION("multiple elements"){
    glue::Element elements;
    elements["A"] = AElement;
    elements["B"] = BElement;
 
    glue::ClassElementContext context;
    context.addElement(elements);

    auto b = context.bind(BElement["create"](42));
    CHECK_NOTHROW(b["setName"]("BB"));
    CHECK(b["description"]()->get<std::string>() == "BB: 42");
    CHECK(b["data"]()->get<int>() == 42);
    CHECK(b["add"](*b)["data"]()->get<int>() == 84);
  }

}
