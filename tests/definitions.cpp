#include <catch2/catch.hpp>
#include <lars/glue/definitions.h>


TEST_CASE("Definitions"){
  using namespace lars;

  struct A {
    void a(double, const A &){}    
    int b(){ return 42; }
  };

  struct B: public A {
    A c()const{ return A(); }
  }; 

  auto extension = std::make_shared<Extension>();
  extension->add_function("f", [](){ return 42; });
  extension->add_function("g", [](int, const std::string &){ });

  auto A_extension = std::make_shared<Extension>();
  A_extension->set_class<A>();
  A_extension->add_function("create", [](){ return A(); });
  A_extension->add_function("a", [](A &a, double d, const A &o){ return a.a(d,o); });
  A_extension->add_function("b", [](A &a){ return a.b(); });

  auto B_extension = std::make_shared<Extension>();
  B_extension->set_class<B>();
  B_extension->set_base_class<A>();
  B_extension->add_function("create", [](int){ return B(); });
  B_extension->add_function("c",[](const B &b){ return b.c(); });

  auto inner_extension = std::make_shared<Extension>();
  inner_extension->add_extension("CA",A_extension);
  inner_extension->add_extension("CB",B_extension);

  extension->add_extension("inner",inner_extension);

  REQUIRE(get_typescript_definitions(*extension) == 
R"(namespace inner {
  class CA{
    a(arg2: number, arg3: CA):void;
    b():number;
    static create():CA;
  }
  class CB extends CA{
    c():CA;
    static create(arg1: number):CB;
  }
}
function f():number;
function g(arg1: number, arg2: string):void;
)");

}


